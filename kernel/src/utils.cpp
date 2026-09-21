#include <utils.hpp>

uint32_t TASK_FINISH_RETURN_ADDRESS = 0;

namespace
{
    inline volatile uint32_t &reg(uint32_t addr)
    {
        return *reinterpret_cast<volatile uint32_t *>(addr);
    }
}

void led_init()
{
    reg(GPFSEL4) = (reg(GPFSEL4) & ~(0b111u << 21)) | (0b001u << 21);
}

void sleep_block(int inters)
{
    for (volatile int i = 0; i < inters; i++)
        ;
}

void led_halt()
{
    while (true)
    {
        sleep_block(500000);
        reg(GPCLR1) = 1u << LED_BIT_IN_BANK;
        sleep_block(500000);
        reg(GPSET1) = 1u << LED_BIT_IN_BANK;
    }
}