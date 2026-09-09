// No libc, no runtime, no operating system underneath -- same
// -ffreestanding -nostdlib situation as freestanding_boot/, just real
// ARM silicon (or QEMU emulating it) instead of x86/QEMU-Multiboot.
// extern "C" on kernel_main so the linker sees the plain symbol name
// boot.s's `bl kernel_main` refers to.
#include <cstdint>

// ---- Given: BCM2835 peripheral addresses -----------------------------
//
// These are hardware facts, not design decisions -- verified against
// the BCM2835 ARM Peripherals datasheet and confirmed working under
// qemu-system-arm's raspi0 machine while this exercise was scaffolded.
// Every Pi peripheral is memory-mapped (unlike x86, which also has a
// separate port-I/O space) -- exactly the *(volatile T*)ADDR = value
// pattern from mock_uart/, against real (or emulated) hardware.

constexpr uint32_t PERIPHERAL_BASE = 0x20000000u;
constexpr uint32_t GPIO_BASE = PERIPHERAL_BASE + 0x200000u;
constexpr uint32_t UART0_BASE = PERIPHERAL_BASE + 0x201000u; // PL011

// GPIO. GPIO47 drives the Pi Zero W's onboard ACT LED, and -- easy to
// get backwards -- it's active-LOW: clear the pin to turn the LED ON,
// set it to turn it OFF (confirmed against the official Pi Zero W
// schematic). Pin 47 falls in the GPFSEL4/GPSET1/GPCLR1 group: BCM2835
// GPIO registers are grouped in banks of ~10-32 pins, and which register
// in a family you use depends on which bank your pin number falls into.
constexpr uint32_t GPFSEL4 = GPIO_BASE + 0x10; // pins 40-49, 3 bits/pin
constexpr uint32_t GPSET1 = GPIO_BASE + 0x20;  // sets a high bit -> pin 32-53 driven high
constexpr uint32_t GPCLR1 = GPIO_BASE + 0x2C;  // sets a high bit -> pin 32-53 driven low
constexpr unsigned LED_GPIO = 47;
constexpr unsigned LED_BIT_IN_BANK = LED_GPIO - 32; // which bit of GPSET1/GPCLR1 -- 15

// UART0 (PL011), a standard ARM PrimeCell UART -- same register shape
// you'd find on plenty of other ARM boards, not BCM2835-specific.
constexpr uint32_t UART0_DR = UART0_BASE + 0x00;   // data register: read/write a byte here
constexpr uint32_t UART0_FR = UART0_BASE + 0x18;   // flag register: status bits, poll this
constexpr uint32_t UART0_IBRD = UART0_BASE + 0x24; // integer part of the baud rate divisor
constexpr uint32_t UART0_FBRD = UART0_BASE + 0x28; // fractional part (6-bit, /64)
constexpr uint32_t UART0_LCRH = UART0_BASE + 0x2C; // line control: word length, FIFO enable, ...
constexpr uint32_t UART0_CR = UART0_BASE + 0x30;   // control: UART/TX/RX enable bits
constexpr uint32_t UART0_ICR = UART0_BASE + 0x44;  // interrupt clear register

constexpr uint32_t mask_bit5 = 0b100000;

namespace
{
    inline volatile uint32_t &reg(uint32_t addr)
    {
        return *reinterpret_cast<volatile uint32_t *>(addr);
    }
}

// ---- TODO: implement everything below ---------------------------------
//
// 1. uart_init(): bring UART0 up. In order: disable it (CR = 0) so
//    nothing half-configured is live while you change settings; clear
//    any pending interrupt state (ICR = 0x7FF, a "clear everything"
//    mask); set the baud rate divisor via IBRD/FBRD (BAUDDIV =
//    UARTCLK / (16 * baud) = IBRD + FBRD/64 -- IBRD=1, FBRD=40 works out
//    close to 115200 baud assuming the Pi's usual 3MHz UART reference
//    clock, and was what this exercise was validated with); set LCRH
//    for 8 data bits, no parity, one stop bit, FIFO enabled; finally
//    enable the UART itself plus TX and RX in CR.
//    Note found while validating this exercise: QEMU's PL011 model
//    doesn't seem to enforce any of the real hardware's transmit
//    backpressure/FIFO-full behavior -- it happily accepted writes with
//    no polling at all during testing. Real hardware isn't guaranteed to
//    be so forgiving. Implement the TX-ready check in uart_putc() below
//    anyway; it's the correct, portable thing to do regardless of what
//    QEMU tolerates.

void uart_init()
{
    reg(UART0_CR) = 0;
    reg(UART0_ICR) = 0x7FF;
    reg(UART0_IBRD) = 1;
    reg(UART0_FBRD) = 40;
    reg(UART0_LCRH) = 0x70;
    reg(UART0_CR) = 0x301;
}

// 2. uart_putc(char c) / uart_puts(const char *s): poll UART0_FR's
//    TXFF bit (bit 5 -- sender's transmit register/FIFO full) until
//    it's clear, then write the byte to UART0_DR. uart_puts just calls
//    uart_putc for every character up to the null terminator.

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

// 3. led_init(): configure GPIO47 as an output. GPFSEL4 packs 10 pins'
//    worth of 3-bit function-select fields into one register -- exactly
//    the bitfield_utils/ read-modify-write pattern, for real this time.
//    Function code 001 = output. Bit position within GPFSEL4 for pin 47
//    is (47 - 40) * 3 = 21.

void led_init()
{
    reg(GPFSEL4) = (reg(GPFSEL4) & ~(0b111u << 21)) | (0b001u << 21);
}

void sleep_block(int inters)
{
    for (volatile int i = 0; i < inters; i++)
        ;
}

// 4. kernel_main(): call uart_init(), uart_puts() a message containing
//    the exact text "hello from pi zero" (check_boot.sh looks for this
//    substring), call led_init(), then loop forever toggling the LED via
//    GPSET1/GPCLR1 (remember: active-low, so GPCLR1 turns it ON) with a
//    software delay between toggles so it's visible to the eye on real
//    hardware -- an empty volatile-counted loop of a few hundred
//    thousand iterations is a fine, simple choice, same idea as any
//    other busy-wait delay you've already written.

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

extern "C" void kernel_main()
{

    constexpr const char *text = "hello from pi zero\0";

    uart_init();
    uart_puts(text);

    led_init();
    led_halt();
}
