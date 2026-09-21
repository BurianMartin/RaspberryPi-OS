# Progress notes

Living notes on where this kernel actually stands: what's done and verified,
what's a deliberate placeholder, what's an open question, and what's next.
Update this as things change — it's meant to stay current, not be a
one-time snapshot.

## Done and verified — real hardware (Pi Zero W)

Phase 1 (first real-hardware pass) is complete: `kernel.img` boots on the
actual Pi Zero W, both tasks run and hand off the LED, and the system
reaches clean shutdown (`led_halt()`) after both finish — the same
behavior confirmed in QEMU, now confirmed for real. Three real bugs only
showed up on hardware, invisible in QEMU the whole time it was being
built:

- **`_start` has to be the physically first thing in `boot.s`.** QEMU's
  `-kernel` loads the ELF and jumps to whatever address `ENTRY(_start)`
  says, regardless of where that code actually sits in the file. Real
  firmware loads the flat `kernel.img` and jumps straight to `0x8000` —
  i.e. to file offset 0, no matter what's there. `init_array_calls` used
  to sit before `_start` in the file; on real hardware that meant it ran
  *first*, with the stack pointer not even set yet. Every single QEMU run
  up to that point had been silently masking this.
- **`.bss` was never being zeroed anywhere.** QEMU zero-initializes guest
  RAM by default; real SDRAM at power-on is not guaranteed to be zero.
  `boot.s` now explicitly zeroes `__bss_start__`..`__bss_end__` as the
  very first thing `_start` does, before anything else runs.
- **The ACT LED is wired active-low on this board** (confirmed directly
  from `bcm2708-rpi-zero-w.dtb`'s `leds/led-act` node: `gpios = <&gpio 47
  1>`, flag `1` = `GPIO_ACTIVE_LOW`) — `GPCLR1`/`GPSET1` were swapped in
  `demo_task`/`StartUpTask` to match. QEMU's `raspi0` model doesn't model
  the LED at all, so this was invisible there by construction.
- **The System Timer `CS` acknowledge** was changed from a hardcoded bit
  (`reg(TIMER_CS) = 1u << 1`) to a write-back (`reg(TIMER_CS) =
  reg(TIMER_CS)`) — acks whatever's actually pending instead of assuming a
  bit position the datasheet was never fully trustworthy about. This
  resolves the open question below.

**The actual root cause of a very long, confusing debugging session**
turned out to be outside the repo entirely: `config.txt` on the SD card
had `disable_commandline_tags=1` set (added while chasing an unrelated
`os_check` tip). That option silently changes the firmware's kernel load
address from `0x8000` to `0x0` — which the linker script has no way to
know about, so every internal symbol reference resolved `0x8000` bytes
too high. The kernel would run its first few instructions correctly, then
self-corrupt the moment it tried to relocate the exception vector table
(written to `0x0`, which — at the wrong load address — was also where the
kernel's own running code physically was). This produced a
content-independent "runs briefly, corrupts itself, dies" pattern that
looked exactly like a code bug and consumed most of a day chasing dozens
of false leads before the actual cause was found. **`config.txt` must not
have `disable_commandline_tags=1` set, ever, for this kernel.** Worth
remembering since it lives on the SD card, not in git, and is trivial to
silently reintroduce.

## Done and verified (in QEMU, `qemu-system-arm -M raspi0`)

- **Boot chain**: `_start` (`boot.s`) sets up the boot stack, copies the
  exception vector table to `0x0000`, sets up a dedicated IRQ-mode stack
  (`set_irq_stack`), runs `.init_array` (see below), then jumps to
  `kernel_main`.
- **Exception vector table** (`interrupt.s`): each of the 8 slots is
  `ldr pc, [pc, #24]` paired with its own absolute address stored 24 bytes
  ahead, and `set_interrupt_vector_table` copies both halves (16 words,
  not 8) to `0x0000` together. This matters because a plain `b label` or
  `ldr pc, =label` breaks once relocated — the encoded offset is only
  correct at the address the code was originally assembled for. Confirmed
  working via direct exception-mode-switch tracing in QEMU.
- **`.init_array` bootstrap** (`init_array_calls` in `boot.s`, bracketed by
  `__init_array_start`/`__init_array_end` in `linker.ld`): walks the
  compiler-generated list of pending global-initializer functions and
  calls each one before `kernel_main` runs — the same thing a hosted C
  runtime does invisibly, hand-written here since none exists. Needed
  because any global whose initial value requires real computation (like
  `USER_PAGE_BASE = (USER_MEMORY_BASE / 4096) + 1`) otherwise silently
  never runs and the global just sits at its `.bss` default of `0`. Took
  three real bugs to get right, worth remembering the shape of each:
  dereferencing the slot before branching to it (`blx` needs the function
  pointer *stored at* the current position, not the position itself), and
  two separate caller-saved-register-clobber bugs (`r1`, then `r0` itself)
  where the loop's own long-lived state was overwritten by the very
  functions it called, since `r0`-`r3` are never guaranteed to survive a
  call. Both loop variables now live in callee-saved registers (`r5`
  current position, `r6` end marker).
- **Context save/restore**: `context_get`/`context_set`/`fill_new_context`
  (`context.s`, `context.cpp`) plus `irq_handler`'s own inline save/restore
  (using the `^` caret suffix to reach a task's real banked `sp`/`lr` from
  IRQ mode without switching modes early). Round-trip tested in isolation
  earlier in the project; now exercised for real by the running scheduler.
- **BCM2835 interrupt controller + System Timer**: `InterruptController`
  (`interrupts.hpp/cpp`) enables timer 1's IRQ line and arms the first
  compare; `IRQ_fire` (`kernel.cpp`) acks (`CS = 1u<<1`) and re-arms
  (`C1 = CLO + INTERRUPT_PERIOD_US`) every tick. A rigorous in-guest
  measurement (logging real `TIMER_CLO` deltas between ticks, not just
  host wall-clock time) confirmed the actual interval is genuinely close
  to the configured period.
- **Physical page allocator** (`MemoryManager`, `memorymanager.hpp/cpp`):
  a bitmap (`page_free_[4096]`, one bit per 4KB page, covering the full
  512MB address range) tracks free/used pages starting at `USER_PAGE_BASE`
  (everything below `_end` — the kernel image itself — is marked reserved
  at construction). `GetPageBaseAddress()` finds and atomically marks a
  free page in one step; `UsePage`/`FreePage` do the word-index/bit-position
  math directly from a page-aligned address. This replaced the earlier
  static `stacks[SCHEDULER_MAX_TASKS][TASK_STACK_SIZE]` array entirely —
  task stacks are real, individually-tracked allocations now, not a fixed
  worst-case reservation.
- **Scheduler** (`scheduler.hpp/cpp`): `CreateAndAddTask` asks the injected
  `MemoryManager` for a page, sets `ctx.sp` to its top, calls
  `fill_new_context` for `pc`/`CPSR`/zeroed registers. `Run()` jumps into
  the head of the queue; `IRQ_fire` re-queues the interrupted task (or
  calls `FreeTask`, if it's marked `done` — see below) and calls `Run()`
  again each tick, giving real round-robin preemption between `StartUpTask`
  and `demo_task`. `FreeTask` recovers a task's original page address by
  rounding its current (drifted) `ctx.sp` down to the nearest 4KB boundary
  — works because each task's stack is confined to exactly one page.
- **Calibrated per-task delays**: tasks read `TIMER_CLO` directly and
  busy-wait against it (refreshing their own `current_time` after each
  print), rather than guessing a loop-iteration count — deterministic
  regardless of host/QEMU execution speed.

## Deliberately temporary — known, chosen, to be redone

- **`InterruptController::EnableIRQs()` calling `enable_irqs()` directly**
  from inside `StartUpTask`. This works and is race-free *because* it only
  ever runs once `StartUpTask` is already the active task (i.e.
  `current_context` is already valid by construction) — but it's worth
  knowing there's a more automatic alternative: `context_set`'s `msr CPSR,
  lr` already clears the IRQ mask as part of jumping into any task (since
  `fill_new_context` sets `CPSR = 0x1F`), so the explicit call is somewhat
  redundant. Not broken, just worth revisiting if the startup-task pattern
  changes.
- **Task stacks are exactly one fixed-size page (4KB), no more, no less.**
  `MemoryManager` allocates by the page, and nothing currently checks
  whether a task's stack usage would overflow that single page — it just
  happens to be enough for what `StartUpTask`/`demo_task` do today. A
  variable-size, general-purpose heap (for `new`/`malloc`, real dynamic
  allocation of arbitrary sizes) is still a distinct, unbuilt next step on
  top of this page-granularity allocator, not the same thing.

## Open, unresolved

- **IRQ stack (`sp_irq`, 4KB) may be slowly leaking.** `IRQ_fire` calls
  `sch.Run()`, which can end in `context_set` — a one-way jump
  (`pop {pc}`) that never returns. That means the call chain
  `irq_handler → IRQ_fire → Run → Execute → context_set` never unwinds:
  each tick's nested call frames stay abandoned on `sp_irq` instead of
  being popped. Hasn't caused an observed crash in the real-hardware run
  described above (tens of ticks, both tasks completing), but the
  mechanism is real and worth checking directly (log `sp` inside
  `irq_handler` over a much longer run) before trusting long-running
  behavior.
- **VFP/floating-point context is unintegrated.** The `VFP` struct exists
  in `context.hpp` but `Scheduler::Execute` explicitly skips it
  (`context_set(task.ctx); // no VPF`). Any task that touches floating
  point will corrupt another task's FP state silently.
- **`IRQ_fire` assumes it's always a timer tick from channel 1, and
  nothing else.** It unconditionally does the scheduler-tick dance
  (`AddTask`/`FreeTask`/`Run()`) and acks with `reg(TIMER_CS) =
  reg(TIMER_CS)` — correct today because channel 1 is the only System
  Timer compare ever armed, and the timer is the only interrupt source
  ever unmasked at the controller (`InterruptController::EnableIRQs()`).
  The moment either changes — a second timer channel for something
  unrelated to scheduling, or any other interrupt source (GPIO, UART,
  etc.) gets enabled — `irq_handler` routes *everything* through this
  same path, and `IRQ_fire` will misidentify it as a scheduler tick and
  only ever ack `TIMER_CS`. Before adding a second interrupt source of any
  kind, `irq_handler`/`IRQ_fire` needs an actual dispatch mechanism —
  read the interrupt controller's pending register(s)
  (`IRQ_BASIC_PENDING`/`IRQ_PENDING_1`/`IRQ_PENDING_2`), and for each bit
  that's set, call a per-source handler (a switch/lookup on which bit it
  is, one handler function per interrupt source) instead of one function
  that assumes it already knows the cause. More than one source can be
  pending at once, so this needs to check every relevant bit each time,
  not stop at the first match.

## Next goal candidates (not yet prioritized against each other)

- Verify/fix the IRQ-stack leak — either by making `Run()`'s tail call
  not descend into a one-way jump from inside `IRQ_fire`, or by confirming
  the leak genuinely doesn't matter within a bounded tick count.
- Build a real general-purpose heap on top of the page allocator (inline
  block headers stored in the arena itself — no separate `new`'d metadata
  nodes, unlike `mock-os`'s version, which can't be ported directly for
  exactly that circularity reason) and wire up `operator new`/`operator
  delete`. `MemoryManager` currently only ever hands out whole 4KB pages,
  which is fine for task stacks but not for arbitrary-sized allocations.
- A real `sleep` **syscall** (SVC-triggered, task actually yields the CPU
  while blocked) rather than the current busy-wait — needs a new vector
  slot (SWI, currently unused), a syscall-number/argument convention, a
  per-task wake-time field, and "pick next" logic that skips still-sleeping
  tasks. Distinct from the calibrated busy-wait delays already in place,
  which don't yield the CPU to anyone else.
- VFP save/restore in the context struct and `context_set`/`irq_handler`.
- Real UART hardware (a USB-to-TTL adapter on GPIO14/15) — the LED-only
  signal used for Phase 1 is exhausted as a debugging tool; the next round
  of real-hardware work (syscalls, MMU) will need actual trace visibility.
- Longer-term, explicitly deferred by design, not forgotten: virtual
  memory/MMU (isolation between tasks — not needed for a working
  scheduler, but a real planned feature, and a natural next layer once a
  real heap exists too), dual-core work once a Pico is acquired,
  filesystems (the one area where using an existing implementation rather
  than writing one from scratch is the deliberate, already-settled
  choice).
