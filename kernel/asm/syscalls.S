#include "syscall_ids.h"

.section .text

.global sleep
.global trap_syscall

malloc:
    ldr r7, =MALLOC_SYSCALL_ID
    svc #0
    bx lr

sleep:
    ldr r7, =SLEEP_SYSCALL_ID
    svc #0
    bx lr