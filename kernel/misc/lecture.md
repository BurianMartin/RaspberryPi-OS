# Lecture: booting the Pi Zero W bare metal

This is the concept lecture for `hello_boot/`, written to be read offline,
without needing a live session to explain it. It assumes everything from
`mock-os` (in particular `freestanding_boot/`'s concepts -- freestanding
execution, entry points, linker scripts -- and `bitfield_utils`/
`mock_uart`'s register-manipulation patterns) but nothing about the
BCM2835 or ARM specifically.

## How the Pi Zero W actually boots

There's no BIOS, no UEFI, no Multiboot handshake here -- the boot chain is
simpler and stranger than the x86 exercise's. Before the ARM core (the
one your code runs on) does anything at all, the SoC's **GPU**
(VideoCore) boots first, on its own, from an on-chip ROM. It reads a
handful of files off the SD card's boot partition -- which has to be an
ordinary FAT32 filesystem, nothing exotic -- `bootcode.bin` and
`start.elf` (proprietary GPU firmware, you download these, you never
write them), `fixup.dat` (memory-split info `start.elf` needs), and a
plain-text `config.txt` you do write, mostly to say which kernel filename
to load. Once GPU-side setup is done, `start.elf` copies your
`kernel.img` -- raw bytes, no ELF header, no metadata -- into RAM at
physical address `0x8000`, and only then does the ARM core start
executing, right there, in ARM (not Thumb) state, Supervisor mode,
interrupts already disabled.

That last part is why `kernel.img` has to be a **flat binary**, not the
ELF you actually link: `start.elf`'s loader has no ELF parser, it just
does a raw memory copy from file offset 0 to `0x8000`. `arm-none-eabi-
objcopy -O binary kernel.elf kernel.img` is what flattens the linked ELF
into that shape -- strips every header, section table, and symbol,
leaving just the bytes that should live in memory starting at `0x8000`,
in order.

## Why QEMU testing uses the ELF, not the `.img`

You'd expect the QEMU-verified step and the real-hardware step to use the
exact same file, mirroring `freestanding_boot/`'s Multiboot flow where
one ELF served both. It doesn't work that way here, and this was actually
discovered empirically while scaffolding this exercise, not something
documented clearly anywhere obvious: `qemu-system-arm -M raspi0 -kernel`
given the flattened `.img` never reaches the loaded code at all --
instruction-level tracing showed execution stuck looping inside what
looks like QEMU's own internal machine-init routine, never once landing
on an address inside the kernel. Given the ELF directly, it works
correctly: QEMU reads the entry point address out of the ELF header
(`0x8000`, same address either way) and jumps there properly. So the
split is: **ELF for QEMU, flattened `.img` for the real SD card** -- not
because they need different code, but because QEMU's loader and the real
Pi's firmware loader are different pieces of software with different
capabilities, and only one of them understands ELF.

## Physical addresses, and why they're just facts to look up

Every BCM2835 peripheral -- GPIO, UART, timers, everything -- is mapped
into the ARM core's physical address space starting at a fixed base,
`0x20000000` on this chip specifically (later Pi models move this base
address; it's not a universal ARM constant, it's a per-SoC decision).
GPIO's registers live at that base plus `0x200000`; UART0's live at that
base plus `0x201000`. These numbers aren't derived from anything, they're
just where the chip's designers put things -- exactly like `mock_uart/`'s
peripheral addresses were given, not designed, just here it's a real
datasheet instead of an exercise's own header comment.

## GPIO: the read-modify-write pattern, for real

`bitfield_utils/`'s whole point -- reading a multi-bit field out of a
register without disturbing its neighbors, writing one back the same way
-- is exactly what configuring a GPIO pin's function requires. BCM2835
packs ten pins' worth of 3-bit function-select fields into each `GPFSELn`
register (`GPFSEL0` covers pins 0-9, `GPFSEL1` covers 10-19, and so on).
GPIO47 falls in `GPFSEL4`, at bit position `(47 - 40) * 3 = 21`. Function
code `001` means "output"; leaving the other 27 bits of that register
alone while changing just those 3 is the read-modify-write pattern,
verbatim.

Once a pin is an output, BCM2835 doesn't make you read-modify-write the
whole register just to flip one bit high or low -- `GPSET`/`GPCLR`
registers exist specifically so a single write, with only the bit you
care about set, drives exactly that pin without touching any other pin's
state at all (writing a 0 to the other bit positions is a no-op by
design, not "clear those pins"). Pins 0-31 use `GPSET0`/`GPCLR0`; pins
32-53 (GPIO47 included) use `GPSET1`/`GPCLR1`, at bit position `47 - 32 =
15` within that second register.

## The Pi Zero W's LED is active-low -- worth internalizing, not just
## memorizing

Clearing GPIO47 turns the LED **on**; setting it turns the LED **off** --
backwards from what "set a bit" intuitively suggests, and different from
some other Pi models' LED wiring (worth double-checking per-board rather
than assuming a pattern carries over). This is a hardware wiring choice,
confirmed against the official Pi Zero W schematic while scaffolding this
exercise, not something derivable from the register interface alone.
Getting this backwards doesn't crash anything or throw an error -- the
LED just does the opposite of what you expected, a classic "technically
working, semantically inverted" bug.

## UART0: PL011, the same peripheral family as freestanding_boot's serial
## port, different bus

`mock_uart/` modeled the status-register/data-register shape; PL011 (ARM's
standard UART core, used across many ARM SoCs, not just this one) is a
real instance of exactly that shape. The flag register's `TXFF` bit (bit
5) says "transmit side full, don't write yet" -- poll it before every
`UART0_DR` write, the same discipline `mock_uart/`'s `try_write_byte`
already taught. Baud rate on PL011 isn't set directly as a number; it's
set as a divisor split into an integer part (`IBRD`) and a 6-bit
fractional part (`FBRD`, representing sixty-fourths), computed from
`BAUDDIV = UARTCLK / (16 * baud_rate)` -- `IBRD=1, FBRD=40` works out to
roughly 115200 baud assuming the Pi's usual 3MHz UART reference clock,
and is what this exercise was validated with.

Worth knowing, found while validating this exercise: QEMU's PL011 model
didn't enforce any of real hardware's transmit backpressure during
testing -- writes to `UART0_DR` succeeded immediately regardless of
`TXFF`, even with the polling check removed entirely. That's a gap in the
emulation, not a property to rely on; real silicon can and does have a
genuinely finite transmit buffer, and skipping the check is a real way to
lose bytes on actual hardware even though QEMU tolerated it.

## What a working run looks like

Confirmed while scaffolding this exercise, with a full reference
implementation:

```
$ make hello_boot_test
...
serial output: hello from pi zero
test_kernel_boots_and_prints_via_serial: PASS
All hello_boot tests passed.
```

The LED half of this exercise has no equivalent QEMU confirmation --
that one you'll only ever see by flashing `build/hello_boot.img` to an SD
card and watching the actual board.
