#pragma once

#include "Conversions.h"
#include "Core/Runtime.h"
#include "Macros.h"
#include "Memory.h"
#include "Utilities.h"
#include "Memory/Range.h"

namespace kernel {

enum class format {
    as_dec,
    as_hex,
};

class StringView;

class StreamingFormatter {
public:
    // clang-format off
    template <typename T, typename Self>
    using enable_if_default_serializable_t = typename
        enable_if<is_same_v<T, Address32>   ||
                  is_same_v<T, Address64>   ||
                  is_integral_v<T>          ||
                  is_pointer_v<T>           ||
                  is_same_v<T, const char*> ||
                  is_same_v<T, StringView>  ||
                  is_same_v<T, format>      ||
                  is_same_v<T, bool>, Self&>::type;
    // clang-format on

    void set_format(format new_format)
    {
        m_format = new_format;
    }

    void append(StringView string);
    void append(const char* string);
    void append(char c);
    void append(bool value);

    template <typename T>
    enable_if_t<is_arithmetic_v<T>> append(T number);

    template <typename T>
    void append(BasicAddress<T> address)
    {
        auto last_format = m_format;
        set_format(format::as_hex);
        append(static_cast<T>(address));
        set_format(last_format);
    }

    template <typename T>
    enable_if_t<is_pointer_v<T>> append(T pointer)
    {
        append(Address(pointer));
    }

protected:
    virtual void write(StringView) = 0;

private:
    format m_format { format::as_dec };
};

class String : public StreamingFormatter {
public:
    static constexpr size_t max_small_string_size = sizeof(ptr_t) * 2 - 1;

    String() = default;
    String(const char* string) { construct_from(string, length_of(string)); }
    String(StringView string);
    String(const String& other) { construct_from(other.data(), other.size()); }
    String(String&& other) { become(forward<String>(other)); }

    String& operator=(StringView string);

    String& operator=(const char* string)
    {
        construct_from(string, length_of(string));

        return *this;
    }

    String& operator=(const String& string)
    {
        construct_from(string.data(), string.size());

        return *this;
    }

    String& operator=(String&& other)
    {
        become(forward<String>(other));

        return *this;
    }

    [[nodiscard]] StringView to_view() const;

    static size_t length_of(const char* string)
    {
        ASSERT(string != nullptr);

        size_t length = 0;

        while (*(string++))
            ++length;

        return length;
    }

    char* data() { return is_small() ? m_small_string : m_big_string.data; }
    [[nodiscard]] const char* data() const { return is_small() ? m_small_string : m_big_string.data; }

    [[nodiscard]] const char* c_string() const { return data(); }

    [[nodiscard]] size_t size() const { return m_size; }

    [[nodiscard]] const char& at(size_t index) const
    {
        ASSERT(index < size());
        return data()[index];
    }

    [[nodiscard]] char& at(size_t index)
    {
        ASSERT(index < size());
        return data()[index];
    }

    [[nodiscard]] char& operator[](size_t index)
    {
        return at(index);
    }

    [[nodiscard]] const char& operator[](size_t index) const
    {
        return at(index);
    }

    [[nodiscard]] char* begin() { return data(); }
    [[nodiscard]] const char* begin() const { return data(); }

    [[nodiscard]] char* end() { return data() + m_size; }
    [[nodiscard]] const char* end() const { return data() + m_size; }

    void pop_back()
    {
        if (m_size == 0)
            return;

        m_size -= 1;

        if (m_size == max_small_string_size) {
            char buffer[max_small_string_size];
            copy_memory(m_big_string.data, buffer, max_small_string_size - 1);
            delete[] m_big_string.data;

            m_small_string[max_small_string_size] = '\0';
            copy_memory(buffer, m_small_string, max_small_string_size);
        } else {
            *end() = '\0';
        }
    }

    [[nodiscard]] bool empty() const { return m_size == 0; }

    using StreamingFormatter::append;
    void append(const String& other);

    String operator+(const String& other)
    {
        String str = *this;
        str.append(other);

        return str;
    }

    String& operator+=(const String& string)
    {
        append(string);
        return *this;
    }

    String operator+(const char* string)
    {
        String str = *this;
        str.append(string);

        return str;
    }

    template <typename T>
    enable_if_default_serializable_t<T, String> operator+=(T value)
    {
        append(value);
        return *this;
    }

    template <typename T>
    enable_if_default_serializable_t<T, String> operator<<(T value)
    {
        if constexpr (is_same_v<T, format>) {
            set_format(value);
        } else {
            append(value);
        }

        return *this;
    }

    String& operator<<(const String& string)
    {
        append(string);
        return *this;
    }

    [[nodiscard]] bool equals(const StringView& string) const;

    [[nodiscard]] bool equals(const String& other) const
    {
        if (size() != other.size())
            return false;

        for (size_t i = 0; i < size(); ++i) {
            if (at(i) != other.at(i))
                return false;
        }

        return true;
    }

    bool operator==(const StringView& string) const { return equals(string); }

    bool operator==(const String& other) const { return equals(other); }

    bool operator!=(const StringView& string) const { return !equals(string); }

    bool operator!=(const String& other) const { return !equals(other); }

    void clear()
    {
        if (is_small())
            m_small_string[0] = '\0';
        else
            delete[] m_big_string.data;

        m_size = 0;
    }

    ~String()
    {
        if (is_small())
            return;

        delete[] m_big_string.data;
    }

private:
    void write(StringView string) override;

    void construct_from(const char* string, size_t length)
    {
        if (length <= max_small_string_size) {
            if (!is_small())
                clear();

            copy_memory(string, m_small_string, length + 1);
        } else {
            if (is_small())
                m_big_string = {};

            if (m_big_string.capacity < length + 1)
                grow_to_fit(length + 1);

            copy_memory(string, m_big_string.data, length);
            m_big_string.data[length] = '\0';
        }

        m_size = length;
    }

    void become(String&& other)
    {
        if (other.is_small()) {
            construct_from(other.data(), other.size());
            other.at(0) = '\0';
            other.m_size = 0;
        } else {
            if (is_small())
                m_big_string = {};

            swap(m_big_string.data, other.m_big_string.data);
            swap(m_big_string.capacity, other.m_big_string.capacity);
            swap(m_size, other.m_size);
        }
    }

    bool is_small() const { return m_size <= max_small_string_size; }

    void grow_to_fit(size_t size)
    {
        // force a reallocation of at least x2
        if (m_size && ((size / m_size) < 2))
            size = m_size * 2;

        if (is_small()) {
            BigString new_buffer = { new char[size], size };
            copy_memory(m_small_string, new_buffer.data, m_size);

            m_big_string = new_buffer;
            return;
        } else {
            auto* old_data = m_big_string.data;
            m_big_string.data = new char[size];
            m_big_string.capacity = size;
            copy_memory(old_data, m_big_string.data, m_size);
            delete[] old_data;
        }
    }

private:
    union {
        struct {
            char* data;
            size_t capacity;
        } m_big_string;

        char m_small_string[max_small_string_size + 1] {};
    };

    using BigString = decltype(m_big_string);

    size_t m_size { 0 };
};

class StringView {
public:
    StringView() = default;

    StringView find_in_range(const Range& range, size_t step = 1) const
    {
        for (auto pointer = range.begin(); pointer + size() < range.end(); pointer += step) {
            if (StringView(Address(pointer).as_pointer<const char>(), size()).equals(*this))
                return { pointer.as_pointer<const char>(), size() };
        }

        return { };
    }

    StringView(const char* string)
        : m_string(string)
        , m_size(String::length_of(string))
    {
    }

    StringView(const String& string)
        : m_string(string.data())
        , m_size(string.size())
    {
    }

    constexpr StringView(const char* string, size_t length)
        : m_string(string)
        , m_size(length)
    {
    }

    constexpr StringView(StringView string, size_t length)
        : m_string(string.data())
        , m_size(length)
    {
    }

    [[nodiscard]] constexpr const char* begin() const { return data(); }
    [[nodiscard]] constexpr const char* end() const { return data() + m_size; }

    [[nodiscard]] constexpr const char* data() const { return m_string; }
    [[nodiscard]] constexpr size_t size() const { return m_size; }

    [[nodiscard]] constexpr bool empty() const { return m_size == 0; }

    [[nodiscard]] constexpr const char& at(size_t i) const
    {
        ASSERT(i < m_size);

        return data()[i];
    }

    [[nodiscard]] constexpr bool equals(StringView other) const
    {
        if (size() != other.size())
            return false;

        for (size_t i = 0; i < size(); ++i) {
            if (at(i) != other.at(i))
                return false;
        }

        return true;
    }

    constexpr bool operator==(const StringView& other) const { return equals(other); }

    constexpr bool operator!=(const StringView& other) const { return !equals(other); }

    friend bool operator==(const char* l, const StringView& r) { return r.equals(l); }

    friend bool operator!=(const char* l, const StringView& r) { return !r.equals(l); }

    template <size_t Size>
    friend bool operator==(const char (&l)[Size], const StringView& r)
    {
        return r.equals(StringView(l, Size));
    }

    template <size_t Size>
    friend bool operator!=(const char (&l)[Size], const StringView& r)
    {
        return !r.equals(StringView(l, Size));
    }

private:
    const char* m_string { nullptr };
    size_t m_size { 0 };
};

inline constexpr StringView operator""_sv(const char* string, size_t size)
{
    return { string, size };
}

inline String::String(StringView string)
{
    construct_from(string.data(), string.size());
}

inline String& String::operator=(StringView string)
{
    construct_from(string.data(), string.size());

    return *this;
}

inline StringView String::to_view() const
{
    return StringView(*this);
}

inline bool String::equals(const StringView& string) const
{
    return string.equals(to_view());
}

inline void StreamingFormatter::append(StringView string)
{
    write(string);
}

inline void StreamingFormatter::append(const char* string)
{
    append(StringView(string));
}

inline void StreamingFormatter::append(char c)
{
    append(StringView(&c, 1));
}

template <typename T>
enable_if_t<is_arithmetic_v<T>> StreamingFormatter::append(T number)
{
    static constexpr size_t buffer_size = 32;
    char number_buffer[buffer_size];
    size_t chars_written = 0;

    if (m_format == format::as_hex) {
        chars_written = to_hex_string(number, number_buffer, buffer_size, false);
    } else {
        chars_written = to_string(number, number_buffer, buffer_size, false);
    }

    append(StringView(number_buffer, chars_written));
}

inline void StreamingFormatter::append(bool value)
{
    static constexpr StringView true_value = "true"_sv;
    static constexpr StringView false_value = "false"_sv;

    append(value ? true_value : false_value);
}

inline void String::append(const String& other)
{
    append(other.to_view());
}

inline void String::write(StringView string)
{
    auto required_size = size() + string.size();

    if ((is_small() && required_size > max_small_string_size)
        || (!is_small() && required_size > m_big_string.capacity))
        grow_to_fit(m_size + string.size() + 1);

    m_size += string.size();
    copy_memory(string.data(), end() - string.size(), string.size());
    *end() = '\0';
}

template <size_t buffer_size = 256>
class StackStringBuilder : public StreamingFormatter {
public:
    static_assert(buffer_size > 1, "Buffer size cannot be smaller than 2");

    [[nodiscard]] StringView to_view() const { return StringView(m_buffer, m_current_offset); }

    char* data() { return m_buffer; }
    const char* data() const { return m_buffer; }

    template <typename T>
    enable_if_default_serializable_t<T, StackStringBuilder> operator+=(T value)
    {
        append(value);
        return *this;
    }

    template <typename T>
    enable_if_default_serializable_t<T, StackStringBuilder> operator<<(T value)
    {
        if constexpr (is_same_v<T, format>) {
            set_format(value);
        } else {
            append(value);
        }

        return *this;
    }

    [[nodiscard]] size_t capacity() const { return buffer_size; }
    [[nodiscard]] size_t size() const { return m_current_offset; }

    [[nodiscard]] size_t size_left() const { return buffer_size - m_current_offset - 1; }

private:
    void write(StringView string) override
    {
        size_t bytes_to_write = string.size() > size_left() ? size_left() : string.size();

        if (bytes_to_write == 0)
            return;

        copy_memory(string.data(), pointer_to_end(), bytes_to_write);
        m_current_offset += bytes_to_write;
        seal();
    }

    void seal() { m_buffer[m_current_offset] = '\0'; }
    char* pointer_to_end() { return m_buffer + m_current_offset; }

private:
    size_t m_current_offset { 0 };
    char m_buffer[buffer_size];
};
}
