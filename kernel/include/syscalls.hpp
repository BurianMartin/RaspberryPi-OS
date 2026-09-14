#pragma once

#include <cstdint>
#include <cstddef>

extern "C" void memcpy(void *dest, const void *src, size_t n);

extern "C" void memset(void *s, int c, size_t n);