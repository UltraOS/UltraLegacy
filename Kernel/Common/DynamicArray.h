#pragma once

#include "Core/Runtime.h"
#include "InitializerList.h"
#include "Macros.h"
#include "Memory.h"
#include "Traits.h"
#include "Utilities.h"

namespace kernel {

template <typename T>
class DynamicArray {
public:
    DynamicArray() = default;

    DynamicArray(std::initializer_list<T> items)
    {
        reserve(items.size());

        for (auto& item : items)
            emplace(item);
    }

    DynamicArray(size_t reserved_count)
    {
        reserve(reserved_count);
    }

    DynamicArray(const DynamicArray& arr)
    {
        if (arr.empty())
            return;

        reserve(arr.size());

        for (size_t i = 0; i < arr.size(); ++i)
            new (&m_data[i]) T(arr[i]);

        m_size = arr.size();
    }

    DynamicArray(DynamicArray&& other)
    {
        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
        swap(m_capacity, other.m_capacity);
    }

    DynamicArray& operator=(const DynamicArray& other)
    {
        if (this == &other)
            return *this;

        if (other.empty()) {
            clear();
            return *this;
        }

        // Since we have to allocate a new buffer anyway, we can simply
        // destroy all the elements, and then copy other into the new buffer.
        if (m_capacity < other.size()) {
            clear();
            reserve(other.size());

            for (size_t i = 0; i < other.size(); ++i)
                new (&m_data[i]) T(other[i]);

            m_size = other.m_size;

            return *this;
        }

        size_t count_to_replace = min(m_size, other.m_size);
        size_t count_to_construct = other.m_size - count_to_replace;
        size_t count_to_destroy = (other.m_size < m_size) ? m_size - other.m_size : 0;

        size_t i = 0;
        for (; i < count_to_replace; ++i)
            m_data[i] = other[i];
        for (; i < count_to_construct; ++i)
            new (&m_data[i]) T(other[i]);
        for (; i < count_to_destroy; ++i)
            m_data[i].~T();

        m_size = other.m_size;

        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other)
    {
        if (this == &other)
            return *this;

        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
        swap(m_capacity, other.m_capacity);

        return *this;
    }

    T& append(const T& item)
    {
        ensure_space();

        return *new (m_data + m_size++) T(item);
    }

    T& append(T&& item)
    {
        ensure_space();

        return *new (m_data + m_size++) T(move(item));
    }

    template <typename... Args>
    T& emplace(Args&&... args)
    {
        ensure_space();

        return *new (m_data + m_size++) T(forward<Args>(args)...);
    }

    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] bool empty() const { return m_size == 0; }
    [[nodiscard]] size_t capacity() const { return m_capacity; }

    T* begin() { return m_data; }
    const T* begin() const { return m_data; }

    T* end() { return m_data + m_size; }
    const T* end() const { return m_data + m_size; }

    T* data() { return m_data; }

    T& at(size_t i)
    {
        ASSERT(i < m_size);
        return m_data[i];
    }

    const T& at(size_t i) const
    {
        ASSERT(i < m_size);
        return m_data[i];
    }

    T& operator[](size_t i) { return at(i); }
    const T& operator[](size_t i) const { return at(i); }

    T& first()
    {
        ASSERT(!empty());
        return m_data[0];
    }

    T& last()
    {
        ASSERT(!empty());
        return m_data[m_size - 1];
    }

    void reserve(size_t count)
    {
        if (!count || count <= m_capacity)
            return;

        T* new_buffer = reinterpret_cast<T*>(new u8[sizeof(T) * count]);

        for (size_t i = 0; i < m_size; ++i) {
            auto& this_elem = at(i);

            new (&new_buffer[i]) T(move(this_elem));
            this_elem.~T();
        }

        m_capacity = count;

        delete[] reinterpret_cast<u8*>(m_data);
        m_data = new_buffer;
    }

    void expand_to(size_t count)
    {
        if (count <= m_size)
            return;

        reserve(count);

        for (size_t i = 0; i < (count - m_size); ++i)
            new (&m_data[m_size + i]) T();

        m_size = count;
    }

    void erase(T* itr)
    {
        ASSERT(itr >= begin() && itr < end());

        erase_at_unchecked(itr - begin());
    }

    void erase_at(size_t i)
    {
        ASSERT(i < m_size);

        erase_at_unchecked(i);
    }

    void clear()
    {
        for (size_t i = 0; i < m_size; ++i)
            m_data[i].~T();

        m_size = 0;
    }

    ~DynamicArray()
    {
        clear();
        delete[] reinterpret_cast<u8*>(m_data);
    }

private:
    void ensure_space()
    {
        if (m_size != m_capacity)
            return;

        reserve(m_capacity + max<size_t>(m_capacity / 2, 2));
    }

    void erase_at_unchecked(size_t i)
    {
        // Move each element one index before current
        for (; i < m_size - 1; ++i)
            m_data[i] = move(m_data[i + 1]);

        // Last element is now in moved-from state, just destroy it
        m_data[m_size - 1].~T();

        // and remove it from the array
        m_size--;
    }

private:
    T* m_data { nullptr };
    size_t m_size { 0 };
    size_t m_capacity { 0 };
};

}
