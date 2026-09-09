# Entry point. The Pi Zero W's GPU firmware (start.elf, not something you
# write) loads this whole image raw into RAM at physical address 0x8000
# and jumps straight there -- no protected-mode dance, no Multiboot
# handshake like the x86 exercise had, just "here's a blob in memory, go."
# QEMU's raspi0 machine, given the ELF directly via -kernel, reads the
# entry point out of the ELF header and jumps to the same address.
#
# CPU state on entry: ARM state (not Thumb), Supervisor mode, IRQ/FIQ
# already disabled by the firmware. No stack has been set up for you.

.section .text.boot
.global _start
_start:
    # TODO: implement.
    #
    # There is no stack yet -- sp is whatever the firmware left it as.
    # Point it somewhere valid before calling anything, including
    # kernel_main: a `bl` instruction stores a return address in lr, and
    # while that alone doesn't need a stack, kernel_main's own local
    # variables and any nested calls it makes will. 0x8000 is a
    # reasonable choice for the stack's initial top: it grows downward
    # from there, away from your code and data which live at and above
    # 0x8000 going up.
    #
    # Once the stack is set up, branch to kernel_main (bl kernel_main).
    #
    # kernel_main will not return in practice -- it loops forever
    # blinking the LED -- but be correct anyway: there's no OS
    # underneath to return to. Loop forever after the call rather than
    # falling off the end into whatever bytes happen to follow.

    mov sp, #0x8000
    bl kernel_main

hang:
    b hang
