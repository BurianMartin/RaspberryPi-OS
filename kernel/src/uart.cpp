#include <uart.hpp>

namespace
{
    inline volatile uint32_t &reg(uint32_t addr)
    {
        return *reinterpret_cast<volatile uint32_t *>(addr);
    }
}

void uart_init()
{
    reg(UART0_CR) = 0;
    reg(UART0_ICR) = 0x7FF;
    reg(UART0_IBRD) = 1;
    reg(UART0_FBRD) = 40;
    reg(UART0_LCRH) = 0x70;
    reg(UART0_CR) = 0x301;
}

void uart_putc_poll(char c)
{
    while ((reg(UART0_FR) & mask_bit5) != 0x0)
        ;
    reg(UART0_DR) = c;
}

bool uart_putc_try(char c)
{
    if ((reg(UART0_FR) & mask_bit5) != 0x0)
    {
        reg(UART0_DR) = c;
        return true;
    }
    return false;
}

void uart_puts(const char *s)
{
    while (*s)
    {
        uart_putc_poll(*s++);
    }
}