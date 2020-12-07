#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class InterruptSafeSpinLock;

class HeapAllocator {
    MAKE_STATIC(HeapAllocator);

public:
    static void initialize();
    static void feed_block(void* ptr, size_t size, size_t chunk_size_in_bytes = 32);

    static void* allocate(size_t bytes);
    static void free(void* ptr);

    struct Stats {
        size_t heap_blocks;
        size_t free_bytes;
        size_t total_bytes;
        size_t calls_to_free;
        size_t calls_to_allocate;
    };

    static Stats stats();

private:
    struct HeapBlockHeader {
        HeapBlockHeader* next;
        u8* data;
        size_t chunk_count;
        size_t chunk_size;
        size_t free_chunks;

        u8* bitmap() { return reinterpret_cast<u8*>(this + 1); }
        u8* begin() const { return data; }
        u8* end() const { return begin() + chunk_count * chunk_size; }

        size_t which_bit(void* ptr) const { return ((reinterpret_cast<u8*>(ptr) - begin()) / chunk_size) * 2; }
        size_t free_bytes() const { return free_chunks * chunk_size; }
        bool contains(void* ptr) const { return ptr <= end() && ptr >= begin(); }
    } static* s_heap_block;

    inline static size_t s_calls_to_allocate;
    inline static size_t s_calls_to_free;

    static InterruptSafeSpinLock s_lock;
};
}
