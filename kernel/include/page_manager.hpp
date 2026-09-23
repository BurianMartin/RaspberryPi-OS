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

class PageManager
{
private:
public:
    PageManager();
    ~PageManager() = default;

    uint32_t AllocatePage();

    uint32_t AllocatePagesConseq(uint32_t count);

    void UsePage(uint32_t page_address);
    void FreePage(uint32_t page_address);
};