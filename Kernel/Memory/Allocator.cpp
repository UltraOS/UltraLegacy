#include "Allocator.h"
#include "Core/Memory.h"
#include "Core/Runtime.h"

namespace kernel {

    Allocator::HeapBlockHeader* Allocator::m_heap;

    void Allocator::initialize()
    {
        static constexpr size_t kernel_base_address = 0xC0100000;
        static constexpr size_t kernel_size         = 3 * MB;
        static constexpr size_t kernel_end_address  = kernel_base_address + kernel_size;
        static constexpr size_t kernel_heap_begin   = kernel_end_address;
        static constexpr size_t kernel_heap_size    = 4 * MB; // one page directory

        // feed the preallocated kernel heap page directory
        feed_block(reinterpret_cast<void*>(kernel_heap_begin), kernel_heap_size);
    }

    void Allocator::feed_block(void* ptr, size_t size, size_t chunk_size_in_bytes)
    {
        auto& new_heap = *reinterpret_cast<HeapBlockHeader*>(ptr);

        if (m_heap)
        {
            new_heap.next = m_heap->next;
            m_heap->next = &new_heap;
        }
        else
        {
            m_heap = &new_heap;
            new_heap.next = nullptr;
        }

        size_t pure_size = size - sizeof(HeapBlockHeader);

        auto chunk_count = pure_size / chunk_size_in_bytes;

        auto bitmap_bytes = 1 + ((chunk_count - 1) / 8);

        size_t data_ptr = reinterpret_cast<size_t>(reinterpret_cast<u8*>(ptr) + sizeof(HeapBlockHeader) + bitmap_bytes);
        size_t alignment_overhead = 0;

        // align data at chunk size
        if (data_ptr % chunk_size_in_bytes)
            alignment_overhead = chunk_size_in_bytes - data_ptr % chunk_size_in_bytes;

        bitmap_bytes += alignment_overhead;

        pure_size -= bitmap_bytes;

        if (pure_size % chunk_size_in_bytes)
            pure_size -= pure_size % chunk_size_in_bytes;

        new_heap.chunk_size  = chunk_size_in_bytes;
        new_heap.chunk_count = pure_size / chunk_size_in_bytes;
        new_heap.free_chunks = new_heap.chunk_count;
        new_heap.data = new_heap.bitmap() + bitmap_bytes;

        memory_set(new_heap.bitmap(), new_heap.bitmap_size(), 0);
    }

    void* Allocator::allocate(size_t bytes)
    {
        bytes += sizeof(AllocationHeader);

        for (auto* heap = m_heap; heap; heap = heap->next)
        {
            if (heap->free_bytes() < bytes)
                continue;

            size_t chunks_needed = 1 + ((bytes - 1) / heap->chunk_size);

            bool allocation_succeeded = false;
            size_t current_free_block = 0;
            size_t at_bit = 0;

            for (size_t i = 0; i < heap->bitmap_size(); ++i)
            {
                auto byte_index = i / 8;

                bool is_free = !(heap->bitmap()[byte_index] & (1 << (i - (i * (byte_index)))));

                if (is_free)
                    ++current_free_block;
                else
                {
                    current_free_block = 0;
                    at_bit = 0;
                    continue;
                }

                if (current_free_block == 1)
                    at_bit = i;

                if (current_free_block == chunks_needed)
                {
                    allocation_succeeded = true;
                    break;
                }
            }

            if (allocation_succeeded)
            {
                // mark as allocated
                for (size_t i = 0; i < chunks_needed; ++i)
                {
                    auto true_index = i + at_bit;

                    auto byte_index = true_index / 8;
                    heap->bitmap()[byte_index] |= heap->bitmap()[byte_index] | (1 << (true_index - (true_index * (byte_index))));
                }

                auto* data = heap->begin() + at_bit * heap->chunk_size;

                new (data) AllocationHeader{ chunks_needed };

                return data + sizeof(AllocationHeader);
            }
        }

        return nullptr;
    }

    void Allocator::free(void* ptr)
    {
        auto& header = *(reinterpret_cast<AllocationHeader*>(ptr) - 1);

        for (auto heap = m_heap; heap; heap = heap->next)
        {
            if (!heap->belongs_to_block(ptr))
                continue;

            auto at_bit = heap->which_bit(ptr);

            // mark as free
            for (size_t i = 0; i < header.chunk_count; ++i)
            {
                auto true_index = i + at_bit;

                auto byte_index = true_index / 8;
                heap->bitmap()[byte_index] &= heap->bitmap()[byte_index] ^ (1 << (true_index - (true_index * (byte_index))));
            }

            break;
        }
    }
}
