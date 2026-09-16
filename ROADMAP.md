# Roadmap

The scope of this project, defined once so it stays a real target rather than
drifting: **a real, POSIX(-ish) operating system for the Raspberry Pi Zero W —
something you flash to an SD card, boot on real hardware, and actually use.**
SSH-able, a walkable filesystem, a shell, real ported software. Not a
showcase kernel — a full OS.

This is a big, multi-phase project, worked on solo. It lives as **one
monorepo** (this repository), not split across many git repos — see
"Project structure" below for why, and when that might change.

For day-to-day state ("what's actually built and verified right now"), see
`PROGRESS.md`. This file is the opposite timescale: the shape of the whole
project, revisited when the plan itself changes, not every session.

## Project structure

Single repo, with internal directories reflecting the eventual module
boundaries (`Kernel/`, `LibC/`, `Userland/`, `Build/`, roughly — exact
layout TBD as it's actually needed). The reasoning: kernel/libc/syscall
boundaries will keep shifting for a long time, and one person is evolving
all of them together — splitting into separate repos now would mean
constantly coordinating "this libc commit needs that kernel commit" across
git histories, for no real benefit. That coordination cost is worth paying
when independent people/teams maintain each piece on separate schedules
(why Linux and glibc are separate projects) — not when it's fundamentally
one project evolving as a unit. SerenityOS (solo-started, reached real
POSIX-ish completeness) stays a monorepo for exactly this reason; Redox OS
is split, but only pays for that with real multi-contributor tooling
(a package/build "cookbook") built specifically to absorb the cost.

**Split something out later, deliberately, when there's a concrete reason**
— e.g. the WiFi driver becoming a large, genuinely separable effort, or
wanting to let something (a libc port, say) be reused independently of this
kernel. Not before.

## Phase 0 — Foundation (mostly done, see `PROGRESS.md`)

- [x] Boot chain, relocated exception vector table, `.init_array` bootstrap
- [x] BCM2835 interrupt controller + System Timer, real periodic ticks
- [x] Context save/restore, preemptive round-robin scheduler
- [x] Physical page allocator (bitmap, page-granularity)

## Phase 1 — First real-hardware pass

Everything above has only ever run in QEMU. Before building much more on
top of it, worth actually flashing `kernel.img` to the real Pi Zero W and
confirming the LED, UART, and the interrupt/scheduler chain all work for
real — this is specifically where the System Timer `CS`-acknowledge
uncertainty (flagged in `PROGRESS.md`) gets resolved one way or the other.
Small in scope, but overdue, and better done now than after several more
phases are built on an unverified foundation.

## Phase 2 — Kernel fundamentals

- [ ] General-purpose heap (`operator new`/`delete`) on top of the page
      allocator — inline block headers, not a port of `mock-os`'s allocator
- [ ] Real syscall mechanism (SVC/software interrupt), a new vector slot
- [ ] Task exit/cleanup actually exercised (`Task::done` currently unset
      anywhere — see `PROGRESS.md`)
- [ ] VFP/floating-point context save and restore

## Phase 3 — Memory protection

- [ ] MMU: page tables, translation, per-task address spaces
- [ ] Real isolation between tasks — a bug in one can no longer corrupt
      another's memory

Deliberately after Phase 2, not before — a flat-mapped kernel with a
working scheduler is the normal state of things at this point, not a
compromise; the MMU is a real layer on top, not a prerequisite.

## Phase 4 — Storage

- [ ] SD card controller driver (EMMC/SDHC registers) — written from
      scratch, same as every other driver in this project
- [ ] Filesystem *format* (FAT32 or similar) — the one deliberate exception
      to "write it yourself": an existing implementation, since this is
      about learning an on-disk spec, not a core OS concept
- [ ] Basic VFS layer, enough to mount and walk a real filesystem

## Phase 5 — POSIX layer

The layer that makes "install OpenSSH" and "feels like Linux" possible at
all, not just aspirational:

- [ ] Port a lightweight existing libc (`newlib` or `musl`) — the libc
      itself is reused, same shape as the filesystem-format decision; the
      syscall backend underneath it is written here, deliberately
- [ ] Syscalls designed *toward* real POSIX semantics from the start —
      `fork`/`exec`/`wait`, signals, real file descriptors, permissions —
      not just whatever one target program happens to need
- [ ] Minimal users/permissions model (even a single hardcoded user is
      enough to start)
- [ ] pty/tty support (needed for an interactive shell session, and later
      for SSH)

## Phase 6 — Userland and a bootable image

- [ ] A real shell
- [ ] Coreutils-equivalents (`ls`, `cat`, etc.) as actual separate programs
      loaded and run, not functions compiled into the kernel image
- [ ] Build/image tooling that assembles kernel + libc + userland into an
      actual bootable SD card image

Worth proving "can we produce a bootable image with *something* real in
userland" early, even minimally, rather than only at the very end.

## Phase 7 — Networking

- [ ] TCP/IP stack, written from scratch — this part is genuinely
      approachable, plenty of hobbyists write their own against the spec
- [ ] **WiFi driver — TBD, unresolved on purpose.** The Pi Zero W has no
      Ethernet at all, only a Broadcom chip (BCM43430) needing a closed
      firmware blob and a genuinely difficult SDIO/mailbox interface to
      drive — a different tier of difficulty from everything else on this
      list, closer to reverse-engineering than implementing a known spec.
      Revisit this deliberately when it's actually next, rather than
      deciding now: write it from scratch, adapt existing open driver
      code (a second exception alongside filesystems), or use a USB
      network adapter instead (a harder driver than most, but a *known*
      kind of hard, against a documented chip).
- [ ] **This phase breaks the QEMU-first workflow.** `raspi0` doesn't
      emulate the WiFi chip at all — from here on, this specific area
      needs real hardware to test against, not a QEMU/real split. Worth
      the pacing expectation now rather than a surprise later.

## Phase 8 — SSH

- [ ] Build/port actual OpenSSH against the Phase 5 POSIX layer and the
      Phase 7 network stack. Not a from-scratch SSH implementation —
      real cryptography is the standard exception most projects, expert
      or hobbyist, don't reimplement themselves, and this project follows
      that same judgment rather than treating it as a gap.

## Not on the critical path, not forgotten

- Dual-core work on a Pico, once one is acquired — a separate board,
  a separate thread of work, doesn't block anything above.
- Repo splitting (WiFi driver, a reusable libc port, etc.) — revisit only
  when a concrete, separable reason actually appears.
