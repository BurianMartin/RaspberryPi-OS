.equ IRQ_STACK_SIZE, 4096

.section .bss
.align 3
irq_stack_bottom:
    .space IRQ_STACK_SIZE
irq_stack_top:

.section .text

.global set_interrupt_vector_table
.global set_irq_stack
.global disable_irqs
.global enable_irqs
.global current_context

enable_irqs:
    mrs r0, CPSR
    bic r0, r0, #0x80
    msr CPSR_c, r0
    bx lr

disable_irqs:
    mrs r0, CPSR
    orr r0, r0, #0x80
    msr CPSR_c, r0
    bx lr

irq_handler:
    sub lr, lr, #4

    ldr ip, =current_context
    ldr ip, [ip]

    stm ip, {r0-r12}
    mrs r1, SPSR
    add r2, ip, #52
    stm r2, {sp, lr}^
    str lr, [ip, #60]
    str r1, [ip, #64]

    bl IRQ_fire
          
    ldr ip, =current_context
    ldr ip, [ip]
          
    add r2, ip, #52
    ldm r2, {sp, lr}^
    ldr r1, [ip, #64]
    msr SPSR_cxsf, r1
    ldr lr, [ip, #60]
    ldm ip, {r0-r12}
    movs pc, lr

general_handler: 
    b hang

hang:
    b hang

interrupt_vector_table:
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    .word general_handler
    .word general_handler
    .word general_handler
    .word general_handler
    .word general_handler
    .word general_handler
    .word irq_handler
    .word general_handler

set_interrupt_vector_table:
    ldr r0, =interrupt_vector_table
    mov r1, #0x0
    ldm r0, {r2-r9}
    stm r1, {r2-r9}
    add r0, r0, #32
    add r1, r1, #32
    ldm r0, {r2-r9}
    stm r1, {r2-r9} 
    bx lr

set_irq_stack:
    mrs r0, CPSR
    bic r1, r0, #0x1F
    orr r1, r1, #0xD2
    msr CPSR_c, r1
    ldr sp, =irq_stack_top
    msr CPSR_c, r0
    bx lr