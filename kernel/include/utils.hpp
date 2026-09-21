#pragma once

#include <cstdint>
#include <peripherals.hpp>

constexpr uint32_t GPFSEL4 = GPIO_BASE + 0x10;
constexpr uint32_t GPSET1 = GPIO_BASE + 0x20;
constexpr uint32_t GPCLR1 = GPIO_BASE + 0x2C;
constexpr unsigned LED_GPIO = 47;
constexpr unsigned LED_BIT_IN_BANK = LED_GPIO - 32;
constexpr uint32_t SCHEDULER_MAX_TASKS = 8;
constexpr uint32_t TASK_STACK_SIZE = 4 * 1024;   // 4Kb
constexpr uint32_t INTERRUPT_PERIOD_US = 500000; // 500ms

extern "C" uint8_t _end[]; // Where the OS memory end and user allocatable memory begins
const uint32_t USER_MEMORY_BASE = reinterpret_cast<uint32_t>(_end);
const uint32_t USER_MEMORY_END = 0x20000000;
const uint32_t USER_PAGE_BASE = (USER_MEMORY_BASE / 4096) + 1;

extern uint32_t TASK_FINISH_RETURN_ADDRESS;

void led_init();

void sleep_block(int inters);

void led_halt();