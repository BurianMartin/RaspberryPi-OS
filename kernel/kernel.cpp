#include <cstdint>
#include <uart.hpp>
#include <utils.hpp>
#include <scheduler.hpp>
#include <interrupts.hpp>
#include <memorymanager.hpp>

namespace
{
    inline volatile uint32_t &reg(uint32_t addr)
    {
        return *reinterpret_cast<volatile uint32_t *>(addr);
    }
}

Scheduler sch;

void demo_task()
{
    const int us_period = (INTERRUPT_PERIOD_US * 2) - 250;

    uint32_t current_time = reg(TIMER_CLO);

    while (true)
    {
        while (reg(TIMER_CLO) < current_time + us_period)
            ;
        uart_puts("Demo task is running\n\0");
        current_time = reg(TIMER_CLO);
    }
}

void StartUpTask()
{
    InterruptController ic;
    ic.EnableIRQs();

    const int us_period = (INTERRUPT_PERIOD_US * 2) - 250;

    uint32_t current_time = reg(TIMER_CLO);

    while (true)
    {
        while (reg(TIMER_CLO) < current_time + us_period)
            ;
        uart_puts("StartUp task up\n\0");
        current_time = reg(TIMER_CLO);
    }
}

extern "C" void IRQ_fire()
{
    reg(TIMER_CS) = 1u << 1;
    reg(TIMER_C1) = reg(TIMER_CLO) + INTERRUPT_PERIOD_US; // Re-arm the next timer interruptto time now + 10ms

    uart_puts("Interrupt fired\n\0");

    if (!current_task->done)
    {
        sch.AddTask(*current_task);
    }
    else
    {
        sch.FreeTask(*current_task);
    }
    sch.Run();
}

extern "C" void kernel_main()
{
    uart_init();

    constexpr const char *text = "hello from pi zero\n\0";

    MemoryManager mm;
    sch = Scheduler(mm);

    if (!sch.CreateAndAddTask(StartUpTask))
    {
        uart_puts("Failed to add startup task\n\0");
    }

    if (!sch.CreateAndAddTask(demo_task))
    {
        uart_puts("Failed to add demo task\n\0");
    }
    uart_puts(text);

    sch.Run();

    led_init();
    led_halt();
}
