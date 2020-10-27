#pragma once

#include "Core/Runtime.h"
#include "Macros.h"
#include "Memory.h"
#include "Traits.h"
#include "Utilities.h"

namespace kernel {

template <typename T>
class DynamicArray {
public:
    DynamicArray() { }

    DynamicArray(size_t initial_capacity) { reserve(initial_capacity); }

    DynamicArray(const DynamicArray& other) { become(other); }

    DynamicArray(DynamicArray&& other) { become(move(other)); }

    DynamicArray& operator=(const DynamicArray& other)
    {
        become(other);

        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other)
    {
        become(forward<DynamicArray>(other));

        return *this;
    }

    void expand_to(size_t count)
    {
        if (count < m_size)
            return;

        reserve(count);

        if (is_trivially_constructible_v<T>)
            zero_memory(address_of(m_size, m_data), (m_capacity - m_size) * sizeof(T));
        else {
            for (size_t i = m_size + !m_size - 1; i < m_capacity; ++i)
                new (address_of(i, m_data)) T();
        }

        m_size = count;
    }

    void reserve(size_t count) { grow_by(count); }

    [[nodiscard]] T& at(size_t index)
    {
        ASSERT(index < m_size);

        T* elems = reinterpret_cast<T*>(m_data);

        return elems[index];
    }

    [[nodiscard]] const T& at(size_t index) const
    {
        ASSERT(index < m_size);

        T* elems = reinterpret_cast<T*>(m_data);

        return elems[index];
    }

    [[nodiscard]] T& last()
    {
        ASSERT(m_size != 0);

        return at(m_size - 1);
    }

    [[nodiscard]] const T& last() const
    {
        ASSERT(m_size != 0);

        return at(m_size - 1);
    }

    [[nodiscard]] const T& operator[](size_t index) const { return at(index); }

    [[nodiscard]] T& operator[](size_t index) { return at(index); }

    [[nodiscard]] T* data() { return reinterpret_cast<T*>(m_data); }

    [[nodiscard]] const T* data() const { return reinterpret_cast<T*>(m_data); }

    T& append(T&& value)
    {
        grow_if_full();

        new (address_of(m_size, m_data)) T(forward<T&&>(value));
        ++m_size;

        return last();
    }

    T& append(const T& value)
    {
        grow_if_full();

        new (address_of(m_size, m_data)) T(value);
        ++m_size;

        return last();
    }

    template <typename... Args>
    T& emplace(Args&&... args)
    {
        grow_if_full();

        new (address_of(m_size, m_data)) T(forward<Args>(args)...);
        ++m_size;

        return last();
    }

    void erase_at(size_t i)
    {
        ASSERT(i < m_size);

        if constexpr (!is_trivially_destructible_v<T>)
            at(i).~T();

        // if it wasn't the last element we have to copy everything
        if (i != m_size - 1)
            move_elements(i + 1, i, m_size - i - 1);

        --m_size;
    }

    size_t size() const { return m_size; }
    bool empty() const { return size() == 0; }

    size_t capcity() const { return m_capacity; }

    ~DynamicArray() { destroy_data(); }

    T* begin() { return data(); }

    T* end() { return data() + m_size; }

    const T* begin() const { return data(); }

    const T* end() const { return data() + m_size; }

    void clear()
    {
        destroy_all();
        m_size = 0;
    }

private:
    void move_elements(size_t from, size_t to, size_t count)
    {
        if constexpr (is_trivially_copyable_v<T>)
            move_memory(address_of(from, m_data), address_of(to, m_data), count * sizeof(T));
        else {
            // TODO: this only works for moving elements backwars, rewrite to support inserts at random locations
            for (size_t i = 0; i < count; ++i) {
                auto index = from + i;

                new (address_of(index - 1, m_data)) T(move(at(index)));

                if constexpr (!is_trivially_destructible_v<T>)
                    at(index).~T();
            }
        }
    }

    void become(const DynamicArray& other)
    {
        destroy_data();
        grow_by(other.size());

        copy_external_elements(other.m_data, m_data, other.size());

        m_size = other.m_size;
    }

    void become(DynamicArray&& other)
    {
        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
        swap(m_capacity, other.m_capacity);
    }

    void copy_external_elements(u8* from, u8* to, size_t count)
    {
        if constexpr (is_trivially_copyable_v<T>)
            copy_memory(from, to, count * sizeof(T));
        else {
            for (size_t i = 0; i < count; ++i)
                new (address_of(i, to)) T(*reinterpret_cast<T*>(address_of(i, from)));
        }
    }

    void grow_if_full()
    {
        if (m_size < m_capacity)
            return;

        grow_by(max(static_cast<decltype(m_capacity)>(1), m_capacity / 2));
    }

    u8* address_of(size_t index, u8* begin) { return begin + (index * sizeof(T)); }

    void destroy_data()
    {
        destroy_all();

        delete[] m_data;
    }

    void grow_by(size_t element_count)
    {
        auto new_capacity = element_count + m_capacity;

        auto* new_data = new u8[new_capacity * sizeof(T)];
        ASSERT((reinterpret_cast<decltype(alignof(T))>(new_data) % alignof(T)) == 0);

        if constexpr (is_trivially_copyable_v<T>)
            copy_memory(m_data, new_data, m_size * sizeof(T));
        else {
            for (size_t i = 0; i < m_size; ++i)
                new (address_of(i, new_data)) T(move(at(i)));
        }

        destroy_data();

        m_data = new_data;
        m_capacity = new_capacity;
    }

    void destroy_all()
    {
        if constexpr (!is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < m_size; ++i)
                at(i).~T();
        }
    }

private:
    size_t m_size { 0 };
    size_t m_capacity { 0 };
    u8* m_data { nullptr };
};
}
