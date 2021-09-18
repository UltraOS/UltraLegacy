#pragma once

namespace kernel {

enum class MemoryOrder : int {
    RELAXED,
    CONSUME,
    ACQUIRE,
    RELEASE,
    ACQ_REL,
    SEQ_CST
};

template <typename T>
class Atomic
{
public:
    Atomic() : m_value(0) { }

    Atomic(T value)
        : m_value(value)
    {
    }

    T load(MemoryOrder) const { return m_value; }
    void store(T val, MemoryOrder) { m_value = val; }

    bool compare_and_exchange(T* expected, T new_value)
    {
        if (m_value == *expected) {
            m_value = new_value;
            return true;
        }

        *expected = m_value;
        return false;
    }

    T fetch_add(T val, MemoryOrder)
    {
        auto tmp = m_value;
        m_value += val;

        return tmp;
    }

    T fetch_subtract(T val, MemoryOrder)
    {
        auto tmp = m_value;
        m_value -= val;

        return tmp;
    }

private:
    T m_value;
};

}
