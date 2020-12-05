#pragma once

#include "Common/Atomic.h"
#include "Common/Macros.h"
#include "Common/Utilities.h"
#include "Core/Runtime.h"

namespace kernel {

template <typename T>
class RefCounterBase {
public:
    virtual void destroy() = 0;
    virtual T* get() = 0;

    void decrement()
    {
        if (--m_counter == 0) {
            destroy();
        }
    }

    void increment()
    {
        ++m_counter;
    }

    size_t reference_count() const
    {
        return m_counter;
    }

    virtual ~RefCounterBase() = default;

private:
    Atomic<size_t> m_counter { 1 };
};

template <typename T>
class OwningRefCounter : public RefCounterBase<T> {
public:
    template <typename... Args>
    explicit OwningRefCounter(Args&& ... args)
    {
        new (m_value) T(forward<Args>(args)...);
    }

    void destroy() override
    {
        get()->~T();
        delete this;
    }

    T* get() override
    {
       return reinterpret_cast<T*>(m_value);
    }

private:
    alignas(T) u8 m_value[sizeof(T)];
};

template <typename T>
class NonOwningRefCounter : public RefCounterBase<T> {
public:
    explicit NonOwningRefCounter(T* ptr)
        : m_ptr(ptr)
    {
    }

    void destroy() override
    {
        delete m_ptr;
        delete this;
    }

    T* get() override
    {
        return m_ptr;
    }

private:
    T* m_ptr;
};

template <typename T>
class RefPtr {
public:
    RefPtr() { }

    template <typename... Args>
    static RefPtr create(Args&&... args)
    {
        auto* ref_counter = new OwningRefCounter<T>(forward<Args>(args)...);

        return RefPtr(ref_counter);
    }

    RefPtr(T* ptr)
        : m_ptr(ptr)
        , m_ref_count(new NonOwningRefCounter(m_ptr))
    {
    }

    RefPtr(const RefPtr& other)
        : m_ptr(other.m_ptr)
        , m_ref_count(other.m_ref_count)
    {
        increment();
    }

    RefPtr(RefPtr&& other)
    {
        swap(m_ptr, other.m_ptr);
        swap(m_ref_count, other.m_ref_count);
    }

    void adopt(T* ptr)
    {
        decrement();

        m_ptr = ptr;
        m_ref_count = new NonOwningRefCounter<T>(ptr);
    }

    T& operator*() const
    {
        ASSERT(m_ptr);

        return *m_ptr;
    }

    T* operator->() const
    {
        ASSERT(m_ptr);

        return m_ptr;
    }

    RefPtr& operator=(T* ptr)
    {
        adopt(ptr);

        return *this;
    }

    RefPtr& operator=(const RefPtr& other)
    {
        decrement();

        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;

        increment();

        return *this;
    }

    RefPtr& operator=(RefPtr&& other)
    {
        swap(m_ptr, other.m_ptr);
        swap(m_ref_count, other.m_ref_count);

        return *this;
    }

    T* get() { return m_ptr; }

    const T* get() const { return m_ptr; }

    bool is_null() const { return get() == nullptr; }

    explicit operator bool() const { return get(); }

    size_t reference_count() const
    {
        ASSERT(m_ref_count != nullptr);
        return m_ref_count->reference_count();
    }

    ~RefPtr()
    {
         decrement();
    }

private:
    explicit RefPtr(RefCounterBase<T>* ref_counter)
    {
        ASSERT(ref_counter != nullptr);

        m_ptr = ref_counter->get();
        m_ref_count = ref_counter;
    }

    void decrement()
    {
        if (m_ref_count)
            m_ref_count->decrement();
    }

    void increment()
    {
        if (m_ref_count)
            m_ref_count->increment();
    }
    
private:
    T* m_ptr { nullptr };
    RefCounterBase<T>* m_ref_count { nullptr };
};
}
