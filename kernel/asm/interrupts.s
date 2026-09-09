.section .text

.global set_interrupt_vector_table

irq_handler: 
    bl signal

general_handler: 
    b hang

hang:
    b hang

interrupt_vector_table:
    b general_handler
    b general_handler
    b general_handler
    b general_handler
    b general_handler
    b general_handler
    b irq_handler
    b general_handler

set_interrupt_vector_table:
    ldr r0, =interrupt_vector_table  
    mov r1, #0x0                     
    ldm r0, {r2-r9}                  
    stm r1, {r2-r9}                  
    bx lr