.section .text.boot

.global _start

_start:
    mov sp, #0x8000
    bl set_interrupt_vector_table
    bl set_irq_stack
    bl kernel_main

hang:
    b hang
