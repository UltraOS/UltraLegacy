#pragma once

#include "Common/Macros.h"
#include "Common/Utilities.h"

namespace kernel {

    template<typename T>
    class RefPtr
    {
    public:
        RefPtr()
        {
        }

        template<typename... Args>
        static RefPtr create(Args&& ... args)
        {
            return RefPtr(new T(forward<Args>(args)...));
        }

        RefPtr(T* ptr)
            : m_ptr(ptr),
              m_ref_count(new size_t(1))
        {
        }

        RefPtr(const RefPtr& other)
            : m_ptr(other.m_ptr),
              m_ref_count(other.m_ref_count)
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
            m_ref_count = new size_t(1);
        }

        T& operator*()
        {
            ASSERT(m_ptr);

            return *m_ptr;
        }

        T* operator->()
        {
            ASSERT(m_ptr);

            return m_ptr;
        }

        RefPtr& operator=(T* ptr)
        {
            adopt(ptr);

            return *this;
        }

        RefPtr& operator=(RefPtr& other)
        {
            decrement();

            m_ptr = other.get();
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

        T* get()
        {
            return m_ptr;
        }

        const T* get() const
        {
            return m_ptr;
        }

        bool is_null() const
        {
            return get() == nullptr;
        }

        operator bool() const
        {
            return get();
        }

        size_t reference_count() const
        {
            return m_ref_count;
        }

        ~RefPtr()
        {
            decrement();
        }

    private:
        void increment()
        {
            if (!get())
                return;

            ++(*m_ref_count);
        }

        void decrement()
        {
            if (!get())
                return;

            if (--(*m_ref_count) == 0)
            {
                delete m_ptr;
                delete m_ref_count;
            }
        }

    private:
        T* m_ptr            { nullptr };
        size_t* m_ref_count { nullptr };
    };
}
