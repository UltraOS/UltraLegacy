#pragma once

namespace kernel {

template <typename T>
class Atomic
{
public:
    Atomic() : m_value(0) { }

    Atomic(T value)
        : m_value(value)
    {
    }

    operator T&()
    {
        return m_value;
    }

    operator const T& () const
    {
        return m_value;
    }

    T load() { return m_value; }

private:
    T m_value;
};

}
