#pragma once

#include "Core/Types.h"
#include "Core/Macros.h"

namespace kernel {

    class Allocator
    {
    public:
        static void initialize();
        static void feed_block(void* ptr, size_t size, size_t chunk_size_in_bytes = 32);

        static void* allocate(size_t bytes);
        static void  free(void* ptr);
    private:
        struct HeapBlockHeader
        {
            HeapBlockHeader* next;
            u8* data;
            size_t chunk_count;
            size_t chunk_size;
            size_t free_chunks;

            u8* bitmap() { return reinterpret_cast<u8*>(this + 1); }
            u8* begin()  { return data; }
            u8* end()    { return begin() + chunk_count * chunk_size; }

            size_t which_bit(void* ptr) { return chunk_count - ((end() - reinterpret_cast<u8*>(ptr)) / chunk_size) - 1; }

            size_t bitmap_size() { return chunk_count * 8; }
            size_t free_bytes()  { return free_chunks * chunk_size; }
            bool belongs_to_block(void* ptr) { return ptr <= end(); }

        } static* m_heap;

        struct AllocationHeader
        {
            size_t chunk_count;
        };
    };
}
