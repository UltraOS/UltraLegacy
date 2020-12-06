#pragma once

#include "Common/Lock.h"
#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Common/RedBlackTree.h"
#include "Common/String.h"
#include "Common/Types.h"

#include "Page.h"

namespace kernel {

class VirtualAllocator {
    MAKE_NONCOPYABLE(VirtualAllocator);

public:
    class Range {
    public:
        Range() = default;

        Range(Address start, size_t length)
            : m_begin(start)
            , m_end(start + length)
        {
            ASSERT(m_begin <= m_end);
        }

        static Range from_two_pointers(Address begin, Address end)
        {
            ASSERT(begin <= end);

            Range r;
            r.m_begin = begin;
            r.m_end = end;

            return r;
        }

        static Range create_empty_at(Address address)
        {
            Range r;
            r.m_begin = address;
            r.m_end = address;

            return r;
        }

        Address begin() const
        {
            return m_begin;
        }

        Address end() const
        {
            return m_end;
        }

        size_t length() const
        {
            return m_end - m_begin;
        }

        void set_length(size_t length)
        {
            m_end = m_begin + length;
        }

        bool contains(Address address) const
        {
            return address >= begin() && address < end();
        }

        bool contains(const Range& other) const
        {
            return contains(other.begin()) && other.end() <= end();
        }

        Range clipped_with_begin(Address address) const
        {
            ASSERT(contains(address) || address == end());

            return from_two_pointers(address, m_end);
        }

        Range clipped_with_end(Address address) const
        {
            ASSERT(contains(address) || address == end());

            return from_two_pointers(m_begin, address);
        }

        void reset_with(Address start, size_t length)
        {
            m_begin = start;
            m_end = start + length;
        }

        template <typename T>
        T* as_pointer()
        {
            return m_begin.as_pointer<T>();
        }

        bool operator<(const Range& other) const
        {
            return m_begin < other.m_begin;
        }

        bool operator==(const Range& other) const
        {
            return m_begin == other.m_begin && m_end == other.m_end;
        }

        bool operator!=(const Range& other) const
        {
            return !this->operator==(other);
        }

        bool empty() const
        {
            return m_begin == m_end;
        }

        explicit operator bool() const
        {
            return !empty();
        }

        bool is_aligned_to(size_t alignment) const
        {
            return (m_begin % alignment) == 0;
        }

        void align_to(size_t alignment)
        {
            auto remainder = m_begin % alignment;
            auto aligned_begin = remainder ? Address(m_begin + (alignment - remainder)) : m_begin;

            ASSERT(aligned_begin < m_end);
            ASSERT(aligned_begin >= m_begin);

            m_begin = aligned_begin;
        }

        Range aligned_to(size_t alignment)
        {
            auto remainder = m_begin % alignment;
            auto aligned_begin = remainder ? Address(m_begin + (alignment - remainder)) : m_begin;

            // Overflow or not enough length to handle alignment
            if (aligned_begin < m_begin || aligned_begin >= m_end) {
                return {};
            }

            return Range::from_two_pointers(aligned_begin, m_end);
        }

        template <typename StreamT>
        friend StreamT& operator<<(StreamT&& logger, const Range& range)
        {
            logger << "start:" << range.begin() << " end:" << range.end() << " length:" << range.length();

            return logger;
        }

    private:
        Address m_begin { nullptr };
        Address m_end { nullptr };
    };

    using RangeIterator = RedBlackTree<Range>::Iterator;

    VirtualAllocator() = default;
    VirtualAllocator(Address base, size_t length);

    void reset_with(Address base, size_t length);

    bool contains(Address address);
    bool contains(const Range& range);

    Range allocate(size_t length, size_t alignment = Page::size);
    Range allocate(Range);
    void deallocate(const Range& range);

    bool is_allocated(Address) const;

    String debug_dump() const;

private:
    void merge_and_emplace(RangeIterator before, RangeIterator after, Range new_range);

private:
    Range m_base_range;

    RedBlackTree<Range> m_allocated_ranges;

    mutable InterruptSafeSpinLock m_lock;
};
}
