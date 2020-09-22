#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel {

enum class MemoryOrder : decltype(__ATOMIC_RELAXED) {
    RELAXED = __ATOMIC_RELAXED,
    CONSUME = __ATOMIC_CONSUME,
    ACQUIRE = __ATOMIC_ACQUIRE,
    RELEASE = __ATOMIC_RELEASE,
    ACQ_REL = __ATOMIC_ACQ_REL,
    SEQ_CST = __ATOMIC_SEQ_CST
};

template <typename T>
class Atomic {
public:
    using order_t = decltype(__ATOMIC_RELAXED);

    Atomic() = default;
    Atomic(T value) : m_value(value) { }

    void store(T value, MemoryOrder order = MemoryOrder::SEQ_CST) volatile ALWAYS_INLINE
    {
        __atomic_store_n(&m_value, value, static_cast<order_t>(order));
    }

    T load(MemoryOrder order = MemoryOrder::SEQ_CST) const volatile ALWAYS_INLINE
    {
        return __atomic_load_n(&m_value, static_cast<order_t>(order));
    }

    bool compare_and_exchange(T* expected, T value) volatile ALWAYS_INLINE
    {
        return __atomic_compare_exchange_n(&m_value,
                                           expected,
                                           value,
                                           false,
                                           static_cast<order_t>(MemoryOrder::ACQ_REL),
                                           static_cast<order_t>(MemoryOrder::ACQUIRE));
    }

    T fetch_add(T value, MemoryOrder order = MemoryOrder::SEQ_CST) volatile ALWAYS_INLINE
    {
        return __atomic_fetch_add(&m_value, value, static_cast<order_t>(order));
    }

    T fetch_subtract(T value, MemoryOrder order = MemoryOrder::SEQ_CST) volatile ALWAYS_INLINE
    {
        return __atomic_fetch_sub(&m_value, value, static_cast<order_t>(order));
    }

    T operator++() ALWAYS_INLINE { return fetch_add(1) + 1; }

    T operator++(int) ALWAYS_INLINE { return fetch_add(1); }

    T operator--() ALWAYS_INLINE { return fetch_subtract(1) + 1; }

    T operator--(int) ALWAYS_INLINE { return fetch_subtract(1); }

    operator T() const volatile ALWAYS_INLINE { return load(); }

    T operator=(T value) volatile ALWAYS_INLINE
    {
        store(value);
        return value;
    }

private:
    T m_value { 0 };
};
}
