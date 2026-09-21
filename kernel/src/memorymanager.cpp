#include <memorymanager.hpp>

uint32_t page_free_[4096];

MemoryManager::MemoryManager()
{
    memset(page_free_, ~0u, sizeof(page_free_));

    uint32_t bytes_to_clear = USER_PAGE_BASE / 8;

    memset(page_free_, 0, bytes_to_clear);
    memset(reinterpret_cast<uint8_t *>(page_free_) + bytes_to_clear,
           static_cast<uint8_t>(0xFF << (USER_PAGE_BASE % 8)), 1);
}

uint32_t MemoryManager::GetPageBaseAddress()
{
    for (int index = USER_PAGE_BASE / 32; index < 4096; index++)
    {
        if (page_free_[index] > 0)
        {
            for (int j = 0; j < 32; j++)
            {
                if ((page_free_[index] & (1u << j)) != 0)
                {
                    page_free_[index] &= ~(1u << j);
                    return static_cast<uint32_t>(index * 32 * 4096 + j * 4096);
                }
            }
        }
    }

    return 0;
}

void MemoryManager::UsePage(uint32_t page_address)
{
    uint32_t index = (page_address / 4096);
    uint32_t bit_pos = index % 32;
    index = index / 32;
    page_free_[index] &= ~(1u << bit_pos);
}

void MemoryManager::FreePage(uint32_t page_address)
{
    uint32_t index = (page_address / 4096);
    uint32_t bit_pos = index % 32;
    index = index / 32;
    page_free_[index] |= (1u << bit_pos);
}
