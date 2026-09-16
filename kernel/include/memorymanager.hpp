#pragma once

#include <cstdint>
#include <utils.hpp>
#include <syscalls.hpp>

struct Page
{
    uint32_t base;
    uint32_t end;
    bool free;
};

extern uint32_t page_free_[4096];

class MemoryManager
{
private:
public:
    MemoryManager();
    ~MemoryManager() = default;

    uint32_t GetPageBaseAddress();

    void UsePage(uint32_t page_address);
    void FreePage(uint32_t page_address);
};