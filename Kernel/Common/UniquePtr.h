#pragma once

#include "Common/Utilities.h"
#include "Core/Runtime.h"

namespace kernel {

template <typename T>
class UniquePtr {
public:
    UniquePtr() = default;

    explicit UniquePtr(T* pointer)
        : m_ptr(pointer)
    {
    }

    template <typename... Args>
    static UniquePtr create(Args&&... args)
    {
        return UniquePtr(new T(forward<Args>(args)...));
    }

    UniquePtr(UniquePtr&& other)
    {
        swap(m_ptr, other.m_ptr);
    }

    UniquePtr& operator=(UniquePtr&& other)
    {
        if (this != &other)
            swap(m_ptr, other.m_ptr);

        return *this;
    }

    [[nodiscard]] T* get() const
    {
        return m_ptr;
    }

    [[nodiscard]] T* operator->() const
    {
        ASSERT(m_ptr != nullptr);
        return m_ptr;
    }

    void reset(T* pointer = nullptr)
    {
        delete m_ptr;
        m_ptr = pointer;
    }

    explicit operator bool() const
    {
        return m_ptr != nullptr;
    }

    ~UniquePtr()
    {
        reset();
    }

private:
    T* m_ptr { nullptr };
};

}