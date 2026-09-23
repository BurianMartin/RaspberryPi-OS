#include <task_mem_manager.hpp>
#include <scheduler.hpp>

TaskMemManager::TaskMemManager(uint32_t stack_addr, uint32_t heap_addr, PageManager *page_manager)
{
    stack_address = stack_addr;
    page_mgr = page_manager;
    heap[heap_index] = heap_addr;
    heap_free = reinterpret_cast<uint8_t *>(heap_addr);
}

void *TaskMemManager::get_heap_bytes(size_t size)
{
    if (size > PAGE_SIZE && ((size / PAGE_SIZE) < (static_cast<size_t>(HEAP_MAX_PAGE_COUNT) - heap_index)))
    {
        page_mgr->AllocatePagesConseq((size / PAGE_SIZE) + 1);
    }
    else if (heap[heap_index] + PAGE_SIZE < reinterpret_cast<uint32_t>(heap_free + size + sizeof(uint32_t)))
    {
        if (heap_index + 1 < HEAP_MAX_PAGE_COUNT)
        {
            heap[++heap_index] = page_mgr->AllocatePage();
        }
    }
    else
    {
        current_task->done = true;
        return nullptr;
    }

    void *result = heap_free;
    *(reinterpret_cast<uint32_t *>(heap_free - sizeof(uint32_t))) = size;
    heap_free += size + sizeof(uint32_t);
    return result;
}

void TaskMemManager::free_ptr(void *)
{
    // uint32_t size = reinterpret_cast<uint32_t>(ptr);
}
