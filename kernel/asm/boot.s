.section .text.boot

.global _start

_start:
    mov sp, #0x8000

    ldr r4, =__bss_start__
    ldr r5, =__bss_end__
    mov r6, #0
bss_clear_loop:
    cmp r4, r5
    beq bss_clear_done
    str r6, [r4]
    add r4, r4, #4
    b bss_clear_loop
bss_clear_done:

    bl set_interrupt_vector_table
    bl set_irq_stack
    bl init_array_calls
    bl kernel_main

hang:
    b hang

init_array_calls:
    ldr R5, =__init_array_start
    ldr R6, =__init_array_end
    push {lr}
_top:
    cmp R5, R6
    beq _bot
    ldr R2, [R5]
    blx R2
    add R5, R5, #4
    b _top

_bot:
    pop {pc}
