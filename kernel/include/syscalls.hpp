#pragma once

#include <cstdint>
#include <cstddef>
#include <syscall_ids.h>

extern "C" void memcpy(void *dest, const void *src, size_t n);

extern "C" void memset(void *s, int c, size_t n);

extern "C" void *malloc(size_t size);

// ----------------------------------------------------------------------------------------

extern "C" void sleep(size_t us);