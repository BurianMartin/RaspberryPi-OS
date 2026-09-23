# malloc / free / sleep checklist

A working reference implementation exists at
`~/tmp/RaspberryPi-OS-reference` (a separate copy, built from this repo's
state on 2026-09-23, not tracked here) — this checklist is the path to get
*this* repo into that same state, written by hand rather than copied over.
Check items off as you go; order matters, each section builds on the last.

## 1. PageManager (`page_manager.hpp`/`.cpp`)

- [ ] Implement `AllocatePagesConseq(count)` for real: scan for `count`
      consecutive free page-bits, mark all of them used together
      (all-or-nothing), return the base address of the first one.
- [ ] Add `FreePagesConseq(base, count)` — the matching bulk release.
- [ ] Add a bounds/alignment guard to `FreePage`/`UsePage`: reject (no-op)
      anything not page-aligned or outside the valid page-index range,
      instead of trusting the caller and writing straight into the bitmap.
- [ ] (Optional cleanup) delete the unused `Page` struct — nothing
      references it.

## 2. TaskMemManager (`task_mem_manager.hpp`/`.cpp`)

- [ ] Fix the chunk header to sit *before* the returned pointer, not
      after — trace through the current `get_heap_bytes` by hand with
      concrete numbers to see where it actually lands.
- [ ] Add an intrusive free list: a small header struct (`size` + `next`)
      written into every chunk; `get_heap_bytes` checks it first
      (first-fit, no splitting needed) before ever bumping `heap_free`
      forward; `free_ptr` pushes the freed chunk onto it.
- [ ] Track how many pages a multi-page allocation actually spans (a
      parallel array alongside `heap[]`), since `AllocatePagesConseq` can
      now return runs longer than one page.
- [ ] Add a `Release()` method that walks every owned page (and the
      stack) and returns them to `PageManager`. Call it explicitly from
      `Scheduler::FreeTask` — **not** from the destructor. Reason: `Task`
      gets copied by value through the ring buffer (`AddTask(Task p)`,
      `tasks_[tail_] = p`) constantly; a destructor that actually freed
      things would fire on every incidental copy, not just real task exit.

## 3. Shared register-access helper

- [ ] Move the `inline volatile uint32_t &reg(uint32_t)` helper (currently
      copy-pasted into a local anonymous namespace in several `.cpp`
      files) into `peripherals.hpp` once, and delete the duplicates.
      Needed from `scheduler.cpp` in the next step.

## 4. Sleep syscall

- [ ] Add `TIMER_C3`'s address to `peripherals.hpp` (System Timer channel
      3 — the other ARM-usable channel, since 0/2 belong to the GPU). Add
      named constants for its `TIMER_CS`/`ENABLE_IRQS_1` bit (bit 3,
      mirroring how channel 1 already uses bit 1).
- [ ] Enable channel 3's IRQ line in `InterruptController::EnableIRQs()`
      alongside channel 1's (nothing needs to *arm* it yet — its compare
      register resets to 0, which can't spuriously match a counter
      already past 0).
- [ ] Add a small fixed-capacity min-heap to `Scheduler`, keyed by wake
      time (an array + sift-up/sift-down, sized `SCHEDULER_MAX_TASKS`).
- [ ] Add `Scheduler::SleepCurrentTask(wake_time)`: push the current task
      into that heap (not the ready queue), arm `TIMER_C3` to the new
      earliest, then call `Run()` — same one-way-jump shape as what
      `irq_handler` already does, never returns to its caller.
- [ ] Add `Scheduler::WakeDueTasks(now)`: pop every entry whose wake time
      has passed (careful — `TIMER_CLO` wraps every ~71 minutes, so
      compare with signed subtraction, not `>=`), `AddTask` each one back
      to the ready queue, then re-arm `TIMER_C3` for whatever's left (or
      leave it unarmed if nothing is).
- [ ] Rewrite `irq_handler` to check *both* `TIMER_CS` bits every entry
      (not assume it's always the tick) — ack only what's actually
      pending, do the tick-specific re-arm+print only if bit 1 is set,
      call `WakeDueTasks` only if bit 3 is set, but *always*
      requeue/free whatever task was actually running, regardless of
      which bit(s) fired. This is also the per-source IRQ dispatch
      `PROGRESS.md` already flags as needed.
- [ ] Fill in `svc_handler`'s `SLEEP_SYSCALL_ID` case: read the requested
      microseconds from `ctx.r0`, compute the wake time, call
      `SleepCurrentTask`.

## 5. malloc/free syscalls

- [ ] `syscalls.S` is missing `.global malloc` — it's currently
      unreachable from the linker.
- [ ] Add a `free` trampoline + `FREE_SYSCALL_ID`, and its `svc_handler`
      case calling `free_ptr`.
- [ ] Fix `svc_handler`'s malloc case to write the returned pointer back
      into `current_task->ctx.r0` — right now the result is computed and
      thrown away, so the caller never actually receives it.

## 6. The one that'll bite you if you skip it

- [ ] In every trampoline in `syscalls.S` (`sleep`/`malloc`/`free`),
      `push {r7}` before loading the syscall ID and `pop {r7}` before
      returning. `r7` is callee-saved per the ARM ABI — the compiler is
      allowed to assume it survives a call, and it will, silently, until
      something that relies on a value surviving *two* syscalls in a row
      (like a loop calling `sleep()` repeatedly) breaks in a way that
      looks like a completely unrelated bug three layers away.

## 7. Verify, don't trust

- [ ] Exercise `sleep()` in a loop (not just once) and `malloc`/`free`
      with a real write-then-readback check.
- [ ] Run under QEMU with `-d int,guest_errors` and grep the log for
      `abort`/`undefined`/`invalid` — a silent hang or a wrong value
      won't show up as a build error.

## Deliberately out of scope, not forgotten

Pages only get returned to `PageManager` in bulk when a task exits, not
incrementally as individual chunks are freed mid-run. Real mid-lifetime
reclaim needs a live-allocation counter per page (increment on carve,
decrement on free, return the page when it hits zero) — worth adding
later if a task's heap usage pattern actually needs it, not before.
