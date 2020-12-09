#pragma once

#include "Common/Types.h"
#include "Core/Runtime.h"

namespace kernel {

template <typename AddrT>
class BasicRange {
public:
    BasicRange() = default;

    BasicRange(AddrT start, size_t length)
            : m_begin(start)
            , m_end(start + length)
    {
        ASSERT(m_begin <= m_end);
    }

    static BasicRange from_two_pointers(AddrT begin, AddrT end)
    {
        ASSERT(begin <= end);

        BasicRange r;
        r.m_begin = begin;
        r.m_end = end;

        return r;
    }

    static BasicRange create_empty_at(AddrT address)
    {
        BasicRange r;
        r.m_begin = address;
        r.m_end = address;

        return r;
    }

    AddrT begin() const
    {
        return m_begin;
    }

    AddrT end() const
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

    bool contains(AddrT address) const
    {
        return address >= begin() && address < end();
    }

    bool contains(const BasicRange& other) const
    {
        return contains(other.begin()) && other.end() <= end();
    }

    BasicRange clipped_with_begin(AddrT address) const
    {
        ASSERT(contains(address) || address == end());

        return from_two_pointers(address, m_end);
    }

    BasicRange clipped_with_end(AddrT address) const
    {
        ASSERT(contains(address) || address == end());

        return from_two_pointers(m_begin, address);
    }

    void reset_with(AddrT start, size_t length)
    {
        m_begin = start;
        m_end = start + length;
    }

    template <typename T>
    T* as_pointer()
    {
        return m_begin.template as_pointer<T>();
    }

    bool operator<(const BasicRange& other) const
    {
        return m_begin < other.m_begin;
    }

    bool operator==(const BasicRange& other) const
    {
        return m_begin == other.m_begin && m_end == other.m_end;
    }

    bool operator!=(const BasicRange& other) const
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
        auto aligned_begin = remainder ? AddrT(m_begin + (alignment - remainder)) : m_begin;

        ASSERT(aligned_begin < m_end);
        ASSERT(aligned_begin >= m_begin);

        m_begin = aligned_begin;
    }

    BasicRange aligned_to(size_t alignment)
    {
        auto remainder = m_begin % alignment;
        auto aligned_begin = remainder ? AddrT(m_begin + (alignment - remainder)) : m_begin;

        // Overflow or not enough length to handle alignment
        if (aligned_begin < m_begin || aligned_begin >= m_end) {
            return {};
        }

        return BasicRange::from_two_pointers(aligned_begin, m_end);
    }

    template <typename StreamT>
    friend StreamT& operator<<(StreamT&& logger, const BasicRange& range)
    {
        logger << "start:" << range.begin() << " end:" << range.end() << " length:" << range.length();

        return logger;
    }

private:
    AddrT m_begin { nullptr };
    AddrT m_end { nullptr };
};

using Range = BasicRange<Address>;
using ShortRange = BasicRange<Address32>;
using LongRange = BasicRange<Address64>;

}