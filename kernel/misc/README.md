# Exercise 0: hello_boot (LED blink + UART hello world)

**Status: Scaffolded, not yet implemented.** `boot.s`'s `_start` never
calls `kernel_main` (falls straight into a halt loop), so
`make hello_boot_test` boots cleanly under QEMU, waits out its 5s
timeout, and correctly reports the expected message never showed up on
the emulated serial port. That's the correct starting state.

Confirmed solvable end-to-end before scaffolding: a full reference
implementation was built and booted under `qemu-system-arm -M raspi0`,
produced `hello from pi zero` on the emulated UART, then reverted back to
this stub -- see `lecture.md` for what was actually validated.

## What it teaches

First real-hardware exercise after `mock-os`: applies `bitfield_utils`'s
register read-modify-write pattern and `mock_uart`'s status/data-register
model to actual (or QEMU-emulated) BCM2835 silicon instead of a mock.
Two independent outputs, each verifiable in a different place:
- **UART0 "hello from pi zero"** -- verifiable in QEMU (`make
  hello_boot_test`), and on real hardware if you have a USB-to-TTL serial
  adapter wired to GPIO14/15 (not required).
- **GPIO47 LED blink** -- has no QEMU-visible signal at all; this is the
  real-hardware-only "it's alive" confirmation, watched with your own
  eyes once flashed to an SD card.

## Toolchain

- `arm-none-eabi-g++`/`gcc` (`-mcpu=arm1176jzf-s -marm`) -- compiles
  `kernel.cpp`/assembles `boot.s`
- `arm-none-eabi-ld` with the hand-written `linker.ld` -- links the ELF
- `arm-none-eabi-objcopy -O binary` -- flattens the ELF into
  `kernel.img`, the format the real Pi's firmware loader needs
- `qemu-system-arm -M raspi0` -- boots it for the automated check

All confirmed present and working on this machine. `make hello_boot_test`
builds and QEMU-checks the UART side; `make hello_boot_img` builds
`build/hello_boot.img`, the file that actually goes on an SD card.

## Files

- `boot.s` -- **TODO**: `_start` -- point `sp` at a stack, `bl
  kernel_main`, halt forever after.
- `kernel.cpp` -- **given**: BCM2835/PL011 peripheral and register
  addresses (hardware facts, verified against the datasheet, not a design
  decision). **TODO**: `uart_init`/`uart_putc`/`uart_puts`, `led_init`,
  and `kernel_main` tying it together. Full contract in the file's
  comments.
- `linker.ld` -- **TODO**: the `SECTIONS` block -- same task as
  `freestanding_boot/linker.ld`, different load address (`0x8000`, not
  `0x100000`) and one new gotcha (see below).
- `check_boot.sh` -- the fixed verification spec, **don't edit**.

## A gotcha specific to this exercise

**`-kernel` needs the ELF, not the flattened `.img`, for QEMU.** Found
while scaffolding this: `qemu-system-arm -M raspi0 -kernel
build/hello_boot.img` never reaches the loaded code at all -- instruction
tracing showed it stuck looping in what looks like QEMU's own internal
machine-init code, never once executing an address inside the kernel.
`qemu-system-arm -M raspi0 -kernel build/hello_boot.elf` (the ELF
directly) works correctly -- QEMU reads the entry point out of the ELF
header and jumps there properly. This only affects QEMU testing;
`build/hello_boot.img` (via `make hello_boot_img`) is still exactly what
real hardware needs on the SD card, unrelated to this.

**GCC's string-literal sections aren't named `.rodata`.** They're named
things like `.rodata.str1.4`. A linker script pattern of exactly
`*(.rodata)` won't match and your string data ends up wherever orphan-
section handling happens to put it -- possibly still working by luck, not
by design. Use `*(.rodata*)`.

## Notes for a fresh session

Nothing implemented yet. `lecture.md` has the full BCM2835 GPIO/UART
register picture and the boot-flow explanation (GPU firmware loading a
raw `kernel.img`, no BIOS/Multiboot involved at all).
