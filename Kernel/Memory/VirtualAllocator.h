#pragma once

#include "Common/DynamicArray.h"
#include "Common/Logger.h"
#include "Common/Macros.h"

namespace kernel {

class VirtualAllocator {
public:
    class Range {
    public:
        Range() = default;
        Range(Address start, Address length);

        Address begin() const;

        template <typename T>
        T* as_pointer()
        {
            return m_start.as_pointer<T>();
        }

        Address end() const;
        size_t  length() const;

        bool contains(Address address);

        void set(Address start, Address length);

        bool operator==(const Range& other) const;

        template <typename LoggerT>
        friend LoggerT& operator<<(LoggerT&& logger, const Range& range)
        {
            logger << "start:" << range.begin() << " end:" << range.end() << " length:" << range.length();

            return logger;
        }

    private:
        Address m_start { nullptr };
        size_t  m_length { 0 };
    };

    VirtualAllocator() = default;
    VirtualAllocator(Address base, size_t length);

    void set_range(Address base, size_t length);

    bool contains(Address address);
    bool contains(const Range& range);
    bool is_allocated(Address address);

    Range allocate_range(size_t length);
    void  deallocate_range(const Range& range);
    void  deallocate_range(Address base_address);

private:
    void return_back_to_free_pool(size_t allocated_index);

private:
    Range m_full_range;

    DynamicArray<Range> m_free_ranges;
    DynamicArray<Range> m_allocated_ranges;
};
}
