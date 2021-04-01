#pragma once

#include "Common/Utilities.h"
#include "Common/StaticArray.h"

namespace kernel {

template <typename T, size_t Size>
class CircularBuffer {
public:
    void enqueue(const T& elem)
    {
        m_buffer[m_end] = elem;
        on_element_enqueued();
    }

    void enqueue(T&& elem)
    {
        m_buffer[m_end] = move(elem);
        on_element_enqueued();
    }

    T dequeue()
    {
        ASSERT(!empty());

        m_enqueued_count--;

        auto& elem = m_buffer[m_begin];
        increment(m_begin);

        return elem;
    }

    constexpr size_t size() const
    {
        return Size;
    }

    size_t enqueued_count() const
    {
        return m_enqueued_count;
    }

    bool empty() const { return m_enqueued_count == 0; }
    bool full() const { return m_enqueued_count == Size; }

private:
    void on_element_enqueued()
    {
        m_enqueued_count++;
        increment(m_end);

        if (m_enqueued_count > Size) {
            m_enqueued_count = Size;
            increment(m_begin);
        }
    }

    void increment(size_t& index)
    {
        if (index == Size - 1)
            index = 0;
        else
            ++index;
    }

private:
    size_t m_end { 0 };
    size_t m_begin { 0 };
    size_t m_enqueued_count { 0 };

    StaticArray<T, Size> m_buffer;
};

}