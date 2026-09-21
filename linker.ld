/* Controls where every section ends up in the final binary -- same job
 * as freestanding_boot/'s linker.ld had for x86, different constraints.
 *
 * TODO: implement the SECTIONS block. Requirements:
 *
 * - Load address 0x8000: the Pi Zero W's firmware always loads a raw
 *   kernel.img at physical address 0x8000 and jumps there. Set this with
 *   `. = 0x8000;` before the first section.
 *
 * - Section order: .text.boot first (so _start -- boot.s's entry point
 *   -- ends up at the very front of the image, at 0x8000 itself), then
 *   .text, .rodata, .data, .bss, same idea as the x86 exercise.
 *
 * - Gotcha specific to this one, found while validating the toolchain
 *   for this exercise: GCC doesn't put string literals in a section
 *   literally named .rodata -- it uses names like .rodata.str1.4. A
 *   wildcard pattern of exactly `*(.rodata)` won't match those and your
 *   string data will end up wherever the linker's orphan-section
 *   handling happens to put it (which may still work by luck, but don't
 *   rely on it). Use `*(.rodata*)` instead, to deliberately catch every
 *   .rodata-family section GCC might emit.
 *
 * - .bss is zero-initialized storage, must take up space in memory but
 *   NOT in the file on disk -- same as before, `*(COMMON) *(.bss)` in
 *   its own output section handles this correctly on its own.
 *
 * ENTRY(_start) is already set below -- it tells the linker (and
 * anything reading the ELF header, including QEMU) which symbol is the
 * actual first instruction, separate from where SECTIONS places things.
 */

ENTRY(_start)

SECTIONS
{ 
    . = 0x8000;
    .text.boot  :   {*(.text.boot)}
    __init_array_start = .;
    .init_array : { *(.init_array) }
    __init_array_end = .;
    .text       :   {*(.text)}
    .rodata     :   {*(.rodata*)}
    .data       :   {*(.data)}
    __bss_start__ = .;
    .bss        :   {*(COMMON) *(.bss)}
    __bss_end__ = .;
    _end = .;
}
