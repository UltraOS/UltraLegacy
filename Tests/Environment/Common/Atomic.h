namespace kernel {

template <typename T>
class Atomic
{
public:
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

private:
    T m_value;
};

}
