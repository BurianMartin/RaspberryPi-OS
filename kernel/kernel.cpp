#include <cstdint>
#include <uart.hpp>
#include <utils.hpp>

extern "C" void kernel_main()
{
    constexpr const char *text = "hello from pi zero\0";

    uart_init();
    uart_puts(text);

    led_init();
    led_halt();
}
