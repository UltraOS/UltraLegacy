#pragma once

#include "Conversions.h"
#include "Core/Runtime.h"
#include "Macros.h"
#include "Memory.h"
#include "Utilities.h"

namespace kernel {

class StringView;

class String {
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

    StringView to_view() const;

    static size_t length_of(const char* string)
    {
        size_t length = 0;

        while (*(string++))
            ++length;

        return length;
    }

    char* data() { return is_small() ? m_small_string : m_big_string.data; }
    const char* data() const { return is_small() ? m_small_string : m_big_string.data; }

    const char* c_string() const { return data(); }

    size_t size() const { return m_size; }

    const char& at(size_t index) const { return data()[index]; }
    char& at(size_t index) { return data()[index]; }

    char* begin() { return data(); }
    const char* begin() const { return data(); }

    char* end() { return data() + m_size; }
    const char* end() const { return data() + m_size; }

    String& append(const String& string) { return append(string.data(), string.size()); }
    String& append(const char* string) { return append(string, length_of(string)); }
    String& append(char c) { return append(&c, 1); }
    String& append(const char* string, size_t length)
    {
        auto required_size = size() + length;

        if ((is_small() && required_size > max_small_string_size)
            || (!is_small() && required_size > m_big_string.capacity))
            grow_to_fit(m_size + length + 1);

        m_size += length;
        copy_memory(string, end() - length, length);
        at(m_size) = '\0';

        return *this;
    }

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


    bool empty() const { return m_size == 0; }

    String operator+(const String& other)
    {
        String str = *this;

        return str.append(other);
    }

    String operator+(const char* string)
    {
        String str = *this;

        return str.append(string);
    }

    String& operator+=(const String& other) { return append(other); }

    String& operator+=(const char* string) { return append(string); }

    bool equals(const StringView& string);

    bool equals(const String& other)
    {
        if (size() != other.size())
            return false;

        for (size_t i = 0; i < size(); ++i) {
            if (at(i) != other.at(i))
                return false;
        }

        return true;
    }

    bool operator==(const StringView& string) { return equals(string); }

    bool operator==(const String& other) { return equals(other); }

    bool operator!=(const StringView& string) { return !equals(string); }

    bool operator!=(const String& other) { return !equals(other); }

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

    constexpr const char* begin() const { return data(); }
    constexpr const char* end() const { return data() + m_size; }

    constexpr const char* data() const { return m_string; }
    constexpr size_t size() const { return m_size; }

    constexpr const char& at(size_t i) const
    {
        ASSERT(i < m_size);

        return data()[i];
    }

    constexpr bool equals(StringView other) const
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
    const char* m_string;
    size_t m_size;
};

inline constexpr StringView operator""_sv(const char* string, size_t size)
{
    return StringView(string, size);
}

inline String::String(StringView view)
{
    construct_from(view.data(), view.size());
}

inline String& String::operator=(StringView view)
{
    construct_from(view.data(), view.size());

    return *this;
}

inline StringView String::to_view() const
{
    return StringView(*this);
}

inline bool String::equals(const StringView& string)
{
    return string.equals(to_view());
}

template <size_t buffer_size = 256>
class StackStringBuilder {
public:
    static_assert(buffer_size > 1, "Buffer size cannot be smaller than 2");

    StringView as_view() const { return StringView(m_bufer, m_current_offset); }

    char* data() { return m_bufer; }

    size_t append(StringView string)
    {
        if (string.size() > size_left())
            return 0;

        copy_memory(string.data(), pointer_to_end(), string.size());
        m_current_offset += string.size();
        seal();

        return string.size();
    }

    size_t append(const char* string) { return append(StringView(string)); }

    template <typename T>
    enable_if_t<is_arithmetic_v<T>, size_t> append(T number)
    {
        size_t chars_written = to_string(number, pointer_to_end(), size_left(), false);
        m_current_offset += chars_written;
        seal();

        return chars_written;
    }

    template <typename T>
    enable_if_t<is_arithmetic_v<T>, size_t> append_hex(T number)
    {
        size_t chars_written = to_hex_string(number, pointer_to_end(), size_left(), false);
        m_current_offset += chars_written;
        seal();

        return chars_written;
    }

    size_t append(char c)
    {
        if (!size_left())
            return 0;

        m_bufer[m_current_offset++] = c;
        seal();

        return 1;
    }

    size_t append(bool value)
    {
        static constexpr StringView true_value = "true"_sv;
        static constexpr StringView false_value = "false"_sv;

        auto value_to_copy = value ? true_value : false_value;

        if (size_left() < value_to_copy.size())
            return 0;

        copy_memory(value_to_copy.data(), pointer_to_end(), value_to_copy.size());
        m_current_offset += value_to_copy.size();
        seal();

        return value_to_copy.size();
    }

    template <typename T>
    enable_if_t<is_pointer_v<T>, size_t> append(T pointer)
    {
        return append_hex(reinterpret_cast<ptr_t>(pointer));
    }

    size_t append(Address address) { return append_hex(static_cast<ptr_t>(address)); }

    template <typename T>
    void operator+=(T value)
    {
        append(value);
    }

    template <typename T>
    StackStringBuilder& operator<<(T value)
    {
        append(value);
        return *this;
    }

    size_t capacity() const { return buffer_size; }
    size_t size() const { return m_current_offset; }

    size_t size_left() const { return buffer_size - m_current_offset - 1; }

private:
    void seal() { m_bufer[m_current_offset] = '\0'; }
    char* pointer_to_end() { return m_bufer + m_current_offset; }

private:
    size_t m_current_offset { 0 };
    char m_bufer[buffer_size];
};
}
