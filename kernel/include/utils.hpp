#pragma once

#include <cstdint>
#include <peripherals.hpp>
#include <syscall_ids.h>

constexpr uint32_t GPFSEL4 = GPIO_BASE + 0x10;
constexpr uint32_t GPSET1 = GPIO_BASE + 0x20;
constexpr uint32_t GPCLR1 = GPIO_BASE + 0x2C;
constexpr unsigned LED_GPIO = 47;
constexpr unsigned LED_BIT_IN_BANK = LED_GPIO - 32;
constexpr uint32_t SCHEDULER_MAX_TASKS = 32;
constexpr uint32_t TASK_STACK_SIZE = 4 * 1024;   // 4Kb
constexpr uint32_t INTERRUPT_PERIOD_US = 500000; // 500ms
constexpr uint8_t HEAP_MAX_PAGE_COUNT = 64;      // 4096*64 = 256kb max heap size per task
constexpr uint32_t PAGE_SIZE = 4096;

extern "C" uint8_t _end[]; // Where the OS memory end and user allocatable memory begins
const uint32_t USER_MEMORY_BASE = reinterpret_cast<uint32_t>(_end);
const uint32_t USER_MEMORY_END = 0x20000000;
const uint32_t USER_PAGE_BASE = (USER_MEMORY_BASE / PAGE_SIZE) + 1;

extern uint32_t TASK_FINISH_RETURN_ADDRESS;

void led_init();

void sleep_block(int inters);

void led_halt();