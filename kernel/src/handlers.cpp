#include <handlers.hpp>

namespace
{
    inline volatile uint32_t &reg(uint32_t addr)
    {
        return *reinterpret_cast<volatile uint32_t *>(addr);
    }
}

extern "C" void svc_handler()
{
    switch (current_task->ctx.r7)
    {
    case SLEEP_SYSCALL_ID:

        break;

    case MALLOC_SYSCALL_ID:
        current_task->mem_mgr.get_heap_bytes(current_task->ctx.r0);
        break;
    default:
        break;
    }
}

extern "C" void irq_handler()
{
    reg(TIMER_CS) = reg(TIMER_CS);
    reg(TIMER_C1) = reg(TIMER_CLO) + INTERRUPT_PERIOD_US; // Re-arm the next timer interruptto time now + 10ms

    uart_puts("Interrupt fired\n\0");

    if (!current_task->done)
    {
        scheduler.AddTask(*current_task);
    }
    else
    {
        scheduler.FreeTask(*current_task);
    }
    scheduler.Run();
}