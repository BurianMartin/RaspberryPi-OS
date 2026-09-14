#include <syscalls.hpp>

extern "C" void memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        ((char *)dest)[i] = ((const char *)src)[i];
    }
}

extern "C" void memset(void *s, int c, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        ((char *)s)[i] = (char)c;
    }
}
