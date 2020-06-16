#pragma once

#include "Traits.h"
#include "Utilities.h"
#include "Memory.h"

namespace kernel {

    template<typename T>
    class DynamicArray
    {
    public:
        DynamicArray()
        {
        }

        DynamicArray(size_t initial_capacity)
        {
            reserve(initial_capacity);
        }

        DynamicArray(const DynamicArray& other)
        {
            become(other);
        }

        DynamicArray(DynamicArray&& other)
        {
            become(move(other));
        }

        DynamicArray& operator=(const DynamicArray& other)
        {
            become(other);

            return *this;
        }

        DynamicArray& operator=(DynamicArray&& other)
        {
            become(other);

            return *this;
        }

        void expand_to(size_t count)
        {
            if (count < m_size)
                return;

            reserve(count);

            if (is_trivially_constructible_v<T>)
                zero_memory(address_of(m_size, m_data), (m_capacity - m_size) * sizeof(T));
            else
            {
                for (size_t i = m_size - 1; i < m_capacity; ++i)
                    new (address_of(i, m_data)) T();
            }

            m_size = count;
        }

        void reserve(size_t count)
        {
            grow_by(count);
        }

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
            ASSERT(m_size);

            return at(m_size - 1);
        }

        [[nodiscard]] const T& last() const
        {
            return at(m_size - 1);
        }

        [[nodiscard]] const T& operator[](size_t index) const
        {
            return at(index);
        }

        [[nodiscard]] T& operator[](size_t index)
        {
            return at(index);
        }

        [[nodiscard]] T* data()
        {
            return reinterpret_cast<T*>(m_data);
        }

        [[nodiscard]] const T* data() const
        {
            return reinterpret_cast<T*>(m_data);
        }

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

        template<typename... Args>
        T& emplace(Args&& ... args)
        {
            grow_if_full();

            new (address_of(m_size, m_data)) T(forward<Args>(args)...);
            ++m_size;

            return last();
        }

        size_t size() const
        {
            return m_size;
        }

        size_t capcity() const
        {
            return m_capacity;
        }

        ~DynamicArray()
        {
            destroy_data();
        }

        T* begin()
        {
            return data();
        }

        T* end()
        {
            return data() + m_size;
        }

        const T* begin() const
        {
            return data();
        }

        const T* end() const
        {
            return data() + m_size;
        }

    private:
        void become(const DynamicArray& other)
        {
            destroy_data();
            grow_by(other.size());

            if constexpr (is_trivially_copyable_v<T>)
                copy_memory(other.m_data, m_data, m_size * sizeof(T));
            else
            {
                for (size_t i = 0; i < other.size(); ++i)
                    new (address_of(i, m_data)) T(other.at(i));
            }

            m_size = other.m_size;
        }

        void become(DynamicArray&& other)
        {
            swap(m_data, other.m_data);
            swap(m_size, other.m_size);
            swap(m_capacity, other.m_capacity);
        }

        void grow_if_full()
        {
            if (m_size < m_capacity)
                return;

            grow_by(max(static_cast<decltype(m_capacity)>(1), m_capacity / 2));
        }

        u8* address_of(size_t index, u8* begin)
        {
            return begin + (index * sizeof(T));
        }

        void destroy_data()
        {
            if constexpr (!is_trivially_destructible_v<T>)
            {
                for (size_t i = 0; i < m_size; ++i)
                    at(i).~T();
            }

            delete[] m_data;
        }

        void grow_by(size_t element_count)
        {
            auto new_capacity = element_count + m_capacity;

            auto* new_data = new u8[new_capacity * sizeof(T)];
            ASSERT((reinterpret_cast<decltype(alignof(T))>(new_data) % alignof(T)) == 0);

            if constexpr (is_trivially_copyable_v<T>)
                copy_memory(m_data, new_data, m_size * sizeof(T));
            else
            {
                for (size_t i = 0; i < m_size; ++i)
                    new (address_of(i, new_data)) T(move(at(i)));
            }

            destroy_data();

            m_data = new_data;
            m_capacity = new_capacity;
        }

    private:
        size_t m_size     { 0 };
        size_t m_capacity { 0 };
        u8*    m_data     { nullptr };
    };
}
