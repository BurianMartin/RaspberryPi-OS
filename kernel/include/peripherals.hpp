#pragma once

#include <cstdint>

// Shared BCM2835 physical base addresses. Every peripheral's own
// registers are computed as an offset from these -- not specific to any
// one peripheral (UART, GPIO, etc.), so they live here rather than
// duplicated inside each peripheral's own header.
constexpr uint32_t PERIPHERAL_BASE = 0x20000000u;
constexpr uint32_t GPIO_BASE = PERIPHERAL_BASE + 0x200000u;
