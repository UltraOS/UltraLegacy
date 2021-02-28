#pragma once

#include "Common/Types.h"

namespace kernel {

template <typename T>
class Span {
public:
    Span() = default;

    Span(T* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    size_t size() const { return m_size; }

    T* data() { return m_data; }
    const T* data() const { return m_data; }

    T* begin() { return m_data; }
    T* end() { return m_data + m_size; }

    bool empty() { return m_size == 0; }

    void reset_with(T* data, size_t size)
    {
        m_data = data;
        m_size = size;
    }

    void reset()
    {
        m_data = nullptr;
        m_size = 0;
    }

    void expand_by(size_t size)
    {
        m_size += size;
    }

private:
    T* m_data;
    size_t m_size;
};

}