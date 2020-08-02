#include "Common/Types.h"

namespace kernel {

enum class MemoryOrder : decltype(__ATOMIC_RELAXED)
{
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
    Atomic(T value) : m_value(value) {}

    void store(T value, MemoryOrder order = MemoryOrder::SEQ_CST) volatile
    {
        __atomic_store_n(&m_value, value, static_cast<order_t>(order));
    }

    T load(MemoryOrder order = MemoryOrder::SEQ_CST) const volatile
    {
        return __atomic_load_n(&m_value, static_cast<order_t>(order));
    }

    operator T() const volatile
    {
        return load();
    }

    T operator=(T value) volatile
    {
        store(value);
        return value;
    }

private:
    T m_value;
};
}
