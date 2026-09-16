.section .text.boot

.global _start


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

_start:
    mov sp, #0x8000
    bl set_interrupt_vector_table
    bl set_irq_stack
    bl init_array_calls
    bl kernel_main

hang:
    b hang
