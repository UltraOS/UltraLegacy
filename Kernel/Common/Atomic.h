#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

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
    MAKE_NONMOVABLE(Atomic);

public:
    using order_t = decltype(__ATOMIC_RELAXED);

    Atomic() = default;
    explicit Atomic(T value)
        : m_value(value)
    {
    }

    void store(T value, MemoryOrder order) volatile ALWAYS_INLINE
    {
        __atomic_store_n(&m_value, value, static_cast<order_t>(order));
    }

    T load(MemoryOrder order) const volatile ALWAYS_INLINE
    {
        return __atomic_load_n(&m_value, static_cast<order_t>(order));
    }

    bool compare_and_exchange(T* expected, T value) volatile ALWAYS_INLINE
    {
        return __atomic_compare_exchange_n(
            &m_value,
            expected,
            value,
            false,
            static_cast<order_t>(MemoryOrder::ACQ_REL),
            static_cast<order_t>(MemoryOrder::ACQUIRE));
    }

    T fetch_add(T value, MemoryOrder order) volatile ALWAYS_INLINE
    {
        return __atomic_fetch_add(&m_value, value, static_cast<order_t>(order));
    }

    T fetch_subtract(T value, MemoryOrder order) volatile ALWAYS_INLINE
    {
        return __atomic_fetch_sub(&m_value, value, static_cast<order_t>(order));
    }

private:
    T m_value { 0 };
};
}
