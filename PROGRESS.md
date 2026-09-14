# Progress notes

Living notes on where this kernel actually stands: what's done and verified,
what's a deliberate placeholder, what's an open question, and what's next.
Update this as things change — it's meant to stay current, not be a
one-time snapshot.

## Done and verified (in QEMU, `qemu-system-arm -M raspi0`)

- **Boot chain**: `_start` (`boot.s`) sets up the boot stack, copies the
  exception vector table to `0x0000`, sets up a dedicated IRQ-mode stack
  (`set_irq_stack`), then jumps to `kernel_main`.
- **Exception vector table** (`interrupt.s`): each of the 8 slots is
  `ldr pc, [pc, #24]` paired with its own absolute address stored 24 bytes
  ahead, and `set_interrupt_vector_table` copies both halves (16 words,
  not 8) to `0x0000` together. This matters because a plain `b label` or
  `ldr pc, =label` breaks once relocated — the encoded offset is only
  correct at the address the code was originally assembled for. Confirmed
  working via direct exception-mode-switch tracing in QEMU.
- **Context save/restore**: `context_get`/`context_set`/`fill_new_context`
  (`context.s`, `context.cpp`) plus `irq_handler`'s own inline save/restore
  (using the `^` caret suffix to reach a task's real banked `sp`/`lr` from
  IRQ mode without switching modes early). Round-trip tested in isolation
  earlier in the project; now exercised for real by the running scheduler.
- **BCM2835 interrupt controller + System Timer**: `InterruptController`
  (`interrupts.hpp/cpp`) enables timer 1's IRQ line and arms the first
  compare; `IRQ_fire` (`kernel.cpp`) acks (`CS = 1u<<1`) and re-arms
  (`C1 = CLO + 10000`) every tick. A rigorous in-guest measurement (logging
  real `TIMER_CLO` deltas between ticks, not just host wall-clock time)
  confirmed the actual interval is genuinely close to 10ms.
- **Scheduler** (`scheduler.hpp/cpp`): `CreateAndAddTask` builds a `Task`
  from a plain function pointer — allocates a stack slot, sets `ctx.sp` to
  the top of it, calls `fill_new_context` for `pc`/`CPSR`/zeroed
  registers. `Run()` jumps into the head of the queue; `IRQ_fire` re-queues
  the interrupted task and calls `Run()` again each tick, giving real
  round-robin preemption between `StartUpTask` and `demo_task`.
- **Calibrated per-task delays**: tasks read `TIMER_CLO` directly and
  busy-wait against it (refreshing their own `current_time` after each
  print), rather than guessing a loop-iteration count — deterministic
  regardless of host/QEMU execution speed.

## Deliberately temporary — known, chosen, to be redone

- **Static per-task stacks** (`stacks[SCHEDULER_MAX_TASKS][TASK_STACK_SIZE]`
  in `scheduler.cpp`, currently 8 tasks × 4KB). Explicitly not the final
  design — the plan is a real heap allocator (`operator new`/`delete`
  backed by an inline-header allocator, not a port of `mock-os`'s
  `heap_allocator`, which can't work here since its own bookkeeping nodes
  are allocated with `new` — circular) and `std::unique_ptr`-based
  ownership once that exists. Deliberately deferred until after the
  scheduler mechanism itself was proven working, not skipped.
- **`InterruptController::EnableIRQs()` calling `enable_irqs()` directly**
  from inside `StartUpTask`. This works and is race-free *because* it only
  ever runs once `StartUpTask` is already the active task (i.e.
  `current_context` is already valid by construction) — but it's worth
  knowing there's a more automatic alternative: `context_set`'s `msr CPSR,
  lr` already clears the IRQ mask as part of jumping into any task (since
  `fill_new_context` sets `CPSR = 0x1F`), so the explicit call is somewhat
  redundant. Not broken, just worth revisiting if the startup-task pattern
  changes.

## Open, unresolved

- **The System Timer `CS` acknowledge bit** (`reg(TIMER_CS) = 1u << 1`)
  is a QEMU-verified-working guess, not a datasheet-confirmed fact —
  documentation was inconsistent and QEMU's own `raspi0` model doesn't
  faithfully reflect the real match/ack behavior. Needs verification on
  real hardware; if ticks stop after one, or fire continuously with no
  gap, try the other bit-position hypothesis first.
- **IRQ stack (`sp_irq`, 4KB) may be slowly leaking.** `IRQ_fire` calls
  `sch.Run()`, which can end in `context_set` — a one-way jump
  (`pop {pc}`) that never returns. That means the call chain
  `irq_handler → IRQ_fire → Run → Execute → context_set` never unwinds:
  each tick's nested call frames stay abandoned on `sp_irq` instead of
  being popped. Hasn't caused an observed crash in ~5-6 second test runs,
  but the mechanism is real and worth checking directly (log `sp` inside
  `irq_handler` over a longer run) before trusting long-running behavior.
- **`GetFreeStack()` never marks a stack as used** — `stack_free_[i]`
  starts `true` for all 8 slots and nothing ever sets any of them `false`.
  Harmless with only 2 tasks (there's always a "free" one to hand out),
  but the moment more tasks are created than there are free stacks, this
  will hand out the same stack twice instead of correctly refusing.
- **`Task::done` is never set to `true` anywhere.** `IRQ_fire` already
  checks it (`if (!current_task->done) sch.AddTask(...)`), but nothing in
  the codebase ever marks a task finished, so every task is implicitly
  assumed to run forever. The "let a task finish and get dropped" path is
  unimplemented, not just untested.
- **VFP/floating-point context is unintegrated.** The `VFP` struct exists
  in `context.hpp` but `Scheduler::Execute` explicitly skips it
  (`context_set(task.ctx); // no VPF`). Any task that touches floating
  point will corrupt another task's FP state silently.
- **Everything above is QEMU-only.** No real-hardware testing has
  happened yet — flashing `kernel.img` to the actual Pi Zero W and
  watching the LED/UART/scheduler behavior for real is still outstanding,
  and is specifically where the `CS`-ack uncertainty needs resolving.

## Next goal candidates (not yet prioritized against each other)

- Verify/fix the IRQ-stack leak — either by making `Run()`'s tail call
  not descend into a one-way jump from inside `IRQ_fire`, or by confirming
  the leak genuinely doesn't matter within a bounded tick count.
- Build the real heap allocator (inline block headers stored in the arena
  itself — no separate `new`'d metadata nodes, unlike `mock-os`'s version)
  and wire up `operator new`/`operator delete`, then migrate task stacks
  off the static array.
- Implement a real "task finished" lifecycle: something sets `done`, and
  the scheduler actually drops a finished task instead of re-queuing it
  forever.
- A real `sleep` **syscall** (SVC-triggered, task actually yields the CPU
  while blocked) rather than the current busy-wait — needs a new vector
  slot (SWI, currently unused), a syscall-number/argument convention, a
  per-task wake-time field, and "pick next" logic that skips still-sleeping
  tasks. Distinct from the calibrated busy-wait delays already in place,
  which don't yield the CPU to anyone else.
- VFP save/restore in the context struct and `context_set`/`irq_handler`.
- First real-hardware test pass (LED, UART, then the full interrupt +
  scheduler chain) — this is the one thing QEMU fundamentally can't
  substitute for, especially for the `CS`-ack question above.
- Longer-term, explicitly deferred by design, not forgotten: virtual
  memory/MMU (isolation between tasks — not needed for a working
  scheduler, but a real planned feature), dual-core work once a Pico is
  acquired, filesystems (the one area where using an existing
  implementation rather than writing one from scratch is the deliberate,
  already-settled choice).
