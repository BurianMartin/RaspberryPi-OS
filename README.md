# pi-zero-os

Bare-metal OS development on a real Raspberry Pi Zero W (BCM2835,
ARM1176JZF-S, ARMv6), following on from
[mock-os](https://github.com/BurianMartin/mock-os), which built the
underlying skills (context switching, memory ownership, MMIO patterns,
freestanding execution, linker control) on host Linux first. This repo is
where those skills meet real hardware.

## Scope

Unlike `mock-os`, nothing here is being deliberately scoped down for
simplicity. The goal is a genuinely complete, working system -- scheduling,
memory management (including enabling the MMU for real, since this board
has one), interrupts, drivers, and eventually multi-core work once a Pico
joins this effort -- not a minimal showcase. The one deliberate exception:
filesystems. Implementing filesystem formats is treated as learning a
specific on-disk spec rather than a core OS concept, so an existing
implementation will be used there rather than writing one from scratch.
Everything else gets built.

## Format

Same shape as `mock-os`, per exercise directory:

- Source files, with given hardware facts (register addresses, verified
  against the datasheet) separated from TODO logic (the actual
  read-modify-write/driver/boot code) in comments.
- `check_boot.sh` (or equivalent) -- a fixed, automated verification
  script using QEMU where possible. **Don't edit it.**
- `README.md` -- status, file contract, gotchas found while scaffolding.
- `lecture.md` -- durable conceptual background, written to be read
  offline.
- A `Makefile` target building and checking that exercise.

## Toolchain

`arm-none-eabi-gcc`/`g++` (`-mcpu=arm1176jzf-s -marm`), `arm-none-eabi-ld`
with hand-written linker scripts, `arm-none-eabi-objcopy` to flatten ELFs
into raw `.img` files for the SD card, `qemu-system-arm -M raspi0` for
QEMU-side verification. All confirmed present and working on this
machine.

QEMU can verify UART output but has no visible representation of GPIO
state -- LED-based checks are real-hardware-only by nature, not a gap in
the test scripts.

## Exercises

| # | Directory | Topic | Status |
|---|-----------|-------|--------|
| 0 | `hello_boot/` | Boot chain, GPIO LED blink, UART hello world | Scaffolded, not implemented |

## Hardware

- Raspberry Pi Zero W (in hand)
- Pico/RP2040 (not yet acquired -- dual-core Cortex-M0+, no MMU, real
  hardware spinlocks; will get its own work once acquired)
- No USB-to-TTL serial adapter yet -- the Zero's micro-USB data port is a
  USB OTG connection, not UART; real serial console access needs a
  separate adapter wired to GPIO14/15, unrelated to either USB port on
  the board.

## Flashing

`make <exercise>_img` builds the flat `.img` file. Copy it, alongside the
downloaded GPU firmware files (`bootcode.bin`, `start.elf`, `fixup.dat`)
and a `config.txt`, onto an SD card's FAT32 boot partition as
`kernel.img`. No special tooling needed for the copy itself -- it's an
ordinary filesystem.
