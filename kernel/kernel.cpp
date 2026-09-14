#include <cstdint>
#include <uart.hpp>
#include <utils.hpp>
#include <interrupts.hpp>
#include <scheduler.hpp>

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

    const int us_period = 4500;

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

    const int us_period = 4500;

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
    reg(TIMER_C1) = reg(TIMER_CLO) + 10000; // Re-arm the next timer interruptto time now + 10ms

    if (!current_task->done)
    {
        sch.AddTask(*current_task);
    }
    sch.Run();
}

extern "C" void kernel_main()
{
    uart_init();

    constexpr const char *text = "hello from pi zero\n\0";

    sch = Scheduler();

    if (!sch.CreateAndAddTask(StartUpTask))
    {
        uart_puts("Failed to add startup task\n\0");
    }

    if (!sch.CreateAndAddTask(demo_task))
    {
        uart_puts("Failed to add demo task\n\0");
    }

    sch.Run();
    uart_puts(text);

    led_init();
    led_halt();
}
