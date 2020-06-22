#pragma once

#include "Common/Macros.h"
#include "Common/Logger.h"
#include "Common/DynamicArray.h"

namespace kernel {

    class VirtualAllocator
    {
    public:
        class Range
        {
        public:
            Range() = default;
            Range(ptr_t start, ptr_t length);

            ptr_t begin() const;

            template<typename T>
            T* as_pointer()
            {
                return reinterpret_cast<T*>(m_start);
            }

            ptr_t end() const;
            size_t length() const;

            bool contains(ptr_t address);

            void set(ptr_t start, ptr_t length);

            bool operator==(const Range& other) const;

            template<typename LoggerT>
            friend LoggerT& operator<<(LoggerT&& logger, const Range& range)
            {
                logger << format::as_hex
                       << "start:" << range.begin()
                       << " end:" << range.end()
                       << " length:" << format::as_dec << range.length();

                return logger;
            }
        private:
            ptr_t  m_start  { 0 };
            size_t m_length { 0 };
        };

        VirtualAllocator() = default;
        VirtualAllocator(ptr_t base, size_t length);

        void set_range(ptr_t base, size_t length);

        bool contains(ptr_t address);
        bool contains(const Range& range);
        bool is_allocated(ptr_t address);

        Range allocate_range(size_t length);
        void deallocate_range(const Range& range);
        void deallocate_range(ptr_t base_address);

    private:
        void return_back_to_free_pool(size_t allocated_index);

    private:
        Range m_full_range;

        DynamicArray<Range> m_free_ranges;
        DynamicArray<Range> m_allocated_ranges;
    };
}
