.section .text

.global context_set
.global context_get

context_set:
    mov ip, R0
    add r2, ip, #52
    ldm r2, {sp, lr}^
    ldr r1, [ip, #64]
    msr SPSR_cxsf, r1
    ldr lr, [ip, #60]
    ldm ip, {r0-r12}
    movs pc, lr

context_get:
    stm R0, {r0-r12, sp, lr, pc}
    str lr, [R0, #60]

    mrs R1, CPSR
    str R1, [R0, #64]
    bx lr