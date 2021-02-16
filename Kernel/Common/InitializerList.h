
// Initialize list *must* be in namespace std and called exactly initializer_list as per C++ standard
namespace std {

template <class T>
class initializer_list {
private:
    constexpr initializer_list(const T* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

public:
    constexpr initializer_list() = default;

    constexpr size_t size() const { return m_size; }
    constexpr const T* begin() const { return m_data; }
    constexpr const T* end() const { return m_data + m_size; }

private:
    const T* m_data { nullptr };
    size_t m_size { 0 };
};

}
