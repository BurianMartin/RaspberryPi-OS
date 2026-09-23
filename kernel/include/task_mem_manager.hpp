#pragma once

#include <cstdint>
#include <utils.hpp>
#include <page_manager.hpp>

class TaskMemManager
{
private:
    uint32_t stack_address;

    uint32_t heap[HEAP_MAX_PAGE_COUNT];
    uint8_t heap_index = 0;

    PageManager *page_mgr = nullptr;
    uint8_t *heap_free = nullptr;

public:
    TaskMemManager(uint32_t stack_addr, uint32_t heap_addr, PageManager *page_manager);
    ~TaskMemManager() = default;

    void *get_heap_bytes(size_t size);

    void free_ptr(void *ptr);
};