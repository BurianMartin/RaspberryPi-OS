#include <cstdint>
#include <uart.hpp>
#include <utils.hpp>
#include <context.hpp>
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
context kernel_end_context;

void demo_task()
{
    const int us_period = (INTERRUPT_PERIOD_US * 2) - 250;

    uint32_t current_time = reg(TIMER_CLO);
    uint32_t base_time = current_time;
    uint32_t runtime_ms = 10000000; // 10 seconds

    while (base_time + runtime_ms > current_time)
    {
        while (reg(TIMER_CLO) < current_time + us_period)
            ;
        reg(GPCLR1) = 1u << LED_BIT_IN_BANK; // active-low LED (per bcm2708-rpi-zero-w.dtb): clear = on
        uart_puts("LED on\n\0");
        current_time = reg(TIMER_CLO);
    }
}

void StartUpTask()
{
    InterruptController::EnableIRQs();

    const int us_period = (INTERRUPT_PERIOD_US * 2) - 250;

    uint32_t current_time = reg(TIMER_CLO);
    uint32_t base_time = current_time;
    uint32_t runtime_ms = 10000000; // 10 seconds

    while (base_time + runtime_ms > current_time)
    {
        while (reg(TIMER_CLO) < current_time + us_period)
            ;
        reg(GPSET1) = 1u << LED_BIT_IN_BANK; // active-low LED: set = off
        uart_puts("LED off\n\0");
        current_time = reg(TIMER_CLO);
    }
}

extern "C" void IRQ_fire()
{
    reg(TIMER_CS) = reg(TIMER_CS);
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
    TASK_FINISH_RETURN_ADDRESS = reinterpret_cast<uint32_t>(task_finished);

    volatile bool bootstrapped = false;

    uart_init();
    led_init();

    constexpr const char *text = "hello from pi zero\n\0";

    MemoryManager mm;
    sch = Scheduler(mm, kernel_end_context);

    if (!sch.CreateAndAddTask(StartUpTask))
    {
        uart_puts("Failed to add startup task\n\0");
    }

    if (!sch.CreateAndAddTask(demo_task))
    {
        uart_puts("Failed to add demo task\n\0");
    }
    uart_puts(text);

    context_get(kernel_end_context);
    uart_puts("Kernel finish context set\n\0");
    if (!bootstrapped)
    {
        bootstrapped = true;
        sch.Run();
    }
    else
    {
        uart_puts("All tasks completed, kernel shutting down\n\0");
        InterruptController::DisableIRQs();
    }

    led_halt();
}
