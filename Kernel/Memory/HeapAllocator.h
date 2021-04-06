#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class InterruptSafeSpinLock;

template <typename T>
class Atomic;

class HeapAllocator {
    MAKE_STATIC(HeapAllocator);

public:
    static constexpr size_t upper_allocation_threshold = 2 * MB;

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

    static Atomic<size_t>& total_free_bytes();

    static Stats stats();

    static bool is_deadlocked();
    static bool is_being_refilled() { return s_is_being_refilled; }
    static bool is_initialized() { return s_heap_block != nullptr; }

private:
    static InterruptSafeSpinLock& allocation_lock();
    static InterruptSafeSpinLock& refill_lock();

    static void refill_if_needed(size_t bytes_left);

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
    inline static bool s_is_being_refilled;
};
}
