#include "Allocator.h"
#include "Core/Memory.h"
#include "Core/Runtime.h"

namespace kernel {

    Allocator::HeapBlockHeader* Allocator::m_heap_block;

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

        if (m_heap_block)
        {
            new_heap.next = m_heap_block->next;
            m_heap_block->next = &new_heap;
        }
        else
        {
            m_heap_block = &new_heap;
            new_heap.next = nullptr;
        }

        size_t pure_size = size - sizeof(HeapBlockHeader);

        auto chunk_count = pure_size / chunk_size_in_bytes;

        auto bitmap_bytes = 1 + ((chunk_count - 1) / 4);

        size_t data_ptr = reinterpret_cast<size_t>(reinterpret_cast<u8*>(ptr) + sizeof(HeapBlockHeader) + bitmap_bytes);
        size_t alignment_overhead = 0;

        // align data at chunk size
        if (data_ptr % chunk_size_in_bytes)
            alignment_overhead = chunk_size_in_bytes - (data_ptr % chunk_size_in_bytes);

        bitmap_bytes += alignment_overhead;

        pure_size -= bitmap_bytes;

        // align data at chunk size
        if (pure_size % chunk_size_in_bytes)
            pure_size -= pure_size % chunk_size_in_bytes;

        new_heap.chunk_size  = chunk_size_in_bytes;
        new_heap.chunk_count = pure_size / chunk_size_in_bytes;
        new_heap.free_chunks = new_heap.chunk_count;
        new_heap.data = new_heap.bitmap() + bitmap_bytes;

        memory_set(new_heap.bitmap(), bitmap_bytes, 0);
    }

    void* Allocator::allocate(size_t bytes)
    {
        for (auto* heap = m_heap_block; heap; heap = heap->next)
        {
            if (heap->free_bytes() < bytes)
                continue;

            size_t chunks_needed = 1 + ((bytes - 1) / heap->chunk_size);

            bool allocation_succeeded = false;
            size_t current_free_block = 0;
            size_t at_bit = 0;

            for (size_t i = 0; i < heap->chunk_count * 2; i += 2)
            {
                auto byte_index = i / 8;
                auto bit_index  = i - 8 * byte_index;

                u8 allocation_mask = (1 << (bit_index + 1)) | (1 << bit_index);
                u8 allocation_id = heap->bitmap()[byte_index] & allocation_mask;

                bool is_free = !allocation_id;

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
                auto true_index = at_bit;

                u8 this_id        = 0;
                u8 left_neighbor  = 0;
                u8 right_neighbor = 0;

                // detect left and right neighbors
                if (true_index)
                {
                    auto left = true_index - 2;
                    auto byte_index = left / 8;
                    auto bit_index  = left - 8 * byte_index;
                    auto& control_byte = heap->bitmap()[byte_index];

                    left_neighbor = control_byte & ((1 << (bit_index + 1)) | ((1 << bit_index)));
                    left_neighbor >>= bit_index;
                }
                if (true_index / 2 < heap->chunk_count)
                {
                    auto right = true_index + (chunks_needed * 2);
                    auto byte_index = right / 8;
                    auto bit_index  = right - 8 * byte_index;

                    auto& control_byte = heap->bitmap()[byte_index];

                    right_neighbor = control_byte & ((1 << (bit_index + 1)) | (1 << bit_index));
                    right_neighbor >>= bit_index;
                }

                // find a unique id for this allocation
                for (; this_id == left_neighbor || this_id == right_neighbor || !this_id; ++this_id);

                // mark as allocated with the id
                for (size_t i = 0; i < chunks_needed * 2; i += 2)
                {
                    auto true_index = at_bit + i;
                    auto byte_index = true_index / 8;
                    auto bit_index  = true_index - 8 * byte_index;

                    auto& control_byte = heap->bitmap()[byte_index];

                    control_byte |= this_id << bit_index;
                }

                heap->free_chunks -= chunks_needed;

                auto* data = heap->begin() + (at_bit / 2) * heap->chunk_size;

                return data;
            }
        }

        return nullptr;
    }

    void Allocator::free(void* ptr)
    {
        for (auto heap = m_heap_block; heap; heap = heap->next)
        {
            if (!heap->contains(ptr))
                continue;

            auto at_bit = heap->which_bit(ptr);

            u8 allocation_id = 0;

            // mark as free
            for (size_t i = at_bit;; i += 2)
            {
                auto byte_index = i / 8;
                auto bit_index  = i - 8 * byte_index;

                auto& control_byte = heap->bitmap()[byte_index];
                auto scaled_id = allocation_id << bit_index;

                if (!allocation_id)
                {
                    u8 allocation_mask = (1 << (bit_index + 1)) | (1 << bit_index);
                    allocation_id = control_byte & allocation_mask;
                    scaled_id = allocation_id;
                    allocation_id >>= bit_index;
                }
                else
                {
                    if (!(control_byte & scaled_id))
                        break;
                }

                control_byte ^= scaled_id;

                heap->free_chunks++;
            }

            break;
        }
    }
}
