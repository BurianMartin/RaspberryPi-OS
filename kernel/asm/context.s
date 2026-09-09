.section .text

.global  context_set
.global  context_get

context_set:
    mov ip, R0
    ldr lr, [ip, #64]
    msr CPSR, lr
    ldm ip, {r0-r11}
    ldr lr, [ip, #56]
    ldr sp, [ip, #52]
    ldr R12, [ip, #60]
    push {R12}
    ldr R12, [ip, #48]
    pop {pc}

context_get:
    stm R0, {r0-r12, sp, lr, pc}

    mrs R1, CPSR
    str R1, [R0, #64]
    bx lr
