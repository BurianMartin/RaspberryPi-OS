#include <interrupts.hpp>

namespace
{
    inline volatile uint32_t &reg(uint32_t addr)
    {
        return *reinterpret_cast<volatile uint32_t *>(addr);
    }

    extern "C" void enable_irqs();

    extern "C" void disable_irqs();
}

void InterruptController::EnableIRQs()
{
    reg(ENABLE_IRQS_1) = 1u << 1;
    reg(TIMER_C1) = reg(TIMER_CLO) + INTERRUPT_PERIOD_US; // Arm the next timer interruptto time now + 10ms
    enable_irqs();
}

void InterruptController::DisableIRQs()
{
    disable_irqs();
}