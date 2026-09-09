.section .text

.global  context_set
.global  context_get

context_set:
    mov ip, R0
    ldr R0, [ip, #0]
    ldr R1, [ip, #64]
    msr CPSR, R1
    ldr R1, [ip, #4]
    ldr R2, [ip, #8]
    ldr R3, [ip, #12]
    ldr R4, [ip, #16]
    ldr R5, [ip, #20]
    ldr R6, [ip, #24]
    ldr R7, [ip, #28]
    ldr R8, [ip, #32]
    ldr R9, [ip, #36]
    ldr R10, [ip, #40]
    ldr R11, [ip, #44]
    ldr lr, [ip, #56]
    ldr sp, [ip, #52]
    ldr R12, [ip, #60]   @ stage pc's saved value in r12 -- ip still valid, r12 not yet given its real value
    push {R12}            @ park the staged pc value on the task's own (just-restored) stack momentarily
    ldr R12, [ip, #48]   @ now safe to give r12 its own real final value; ip no longer needed after this
    pop {pc}              @ retrieve the staged pc value and jump -- this is the actual resume

context_get:

    str R0, [R0, #0] 
    str R1, [R0, #4]  
    str R2, [R0, #8] 
    str R3, [R0, #12] 
    str R4, [R0, #16] 
    str R5, [R0, #20] 
    str R6, [R0, #24] 
    str R7, [R0, #28] 
    str R8, [R0, #32] 
    str R9, [R0, #36] 
    str R10, [R0, #40] 
    str R11, [R0, #44] 
    str R12, [R0, #48] 
    str sp, [R0, #52] 
    str lr, [R0, #56] 
    str pc, [R0, #60] 
    mrs R1, CPSR
    str R1, [R0, #64] 
    bx lr
