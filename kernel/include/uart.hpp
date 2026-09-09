#pragma once

#include <cstdint>
#include <peripherals.hpp>

constexpr uint32_t UART0_BASE = PERIPHERAL_BASE + 0x201000u;

constexpr uint32_t UART0_DR = UART0_BASE + 0x00;
constexpr uint32_t UART0_FR = UART0_BASE + 0x18;
constexpr uint32_t UART0_IBRD = UART0_BASE + 0x24;
constexpr uint32_t UART0_FBRD = UART0_BASE + 0x28;
constexpr uint32_t UART0_LCRH = UART0_BASE + 0x2C;
constexpr uint32_t UART0_CR = UART0_BASE + 0x30;
constexpr uint32_t UART0_ICR = UART0_BASE + 0x44;

constexpr uint32_t mask_bit5 = 0b100000;

void uart_init();

void uart_putc_poll(char c);

bool uart_putc_try(char c);

void uart_puts(const char *s);