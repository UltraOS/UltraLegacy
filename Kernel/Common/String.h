#pragma once

#include "Memory.h"
#include "Utilities.h"

namespace kernel {

class StringView;

class String {
public:
    static constexpr size_t max_small_string_size = 7;

    String() = default;

    String(const char* string) { construct_from(string, length_of(string)); }
    String(const String& other) { construct_from(other.data(), other.size()); }
    String(String&& other) { become(forward<String>(other)); }

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

    String(StringView view);

    static size_t length_of(const char* string)
    {
        size_t length = 0;

        while (*(string++))
            ++length;

        return length;
    }

    char* data() { return is_small() ? m_small_string : m_big_string.data; }
    const char* data() const { return is_small() ? m_small_string : m_big_string.data; }

    size_t size() const { return m_size; }

    const char& at(size_t index) const { return data()[index]; }
    char& at(size_t index) { return data()[index]; }

    char* begin() { return data(); }
    const char* begin() const { return data(); }

    char* end() { return data() + m_size; }
    const char* end() const { return data() + m_size; }

    String& append(const String& string) { return append(string.data(), string.size()); }
    String& append(const char* string) { return append(string, length_of(string)); }
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

    bool operator==(const char* string)
    {
        size_t str_length = length_of(string);

        if (size() != str_length)
            return false;

        for (size_t i = 0; i < str_length; ++i)
        {
            if (at(i) != string[i])
                return false;
        }

        return true;
    }

    bool operator==(const String& other)
    {
        if (size() != other.size())
            return false;

        for (size_t i = 0; i < size(); ++i)
        {
            if (at(i) != other.at(i))
                return false;
        }

        return true;
    }

    bool operator!=(const char* string) { return !(*this == string); }

    bool operator!=(const String& other) { return !(*this == other); }

    void clear()
    {
        m_size = 0;

        if (is_small())
            m_small_string[0] = '\0';
        else
            delete[] m_big_string.data;
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
        if (other.is_small())
            construct_from(other.data(), other.size());
        else {
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
        if (is_small()) {
            BigString new_buffer = { new char[size], size };
            copy_memory(m_small_string, new_buffer.data, m_size);

            m_big_string = new_buffer;
            return;
        } else {
            auto* old_data        = m_big_string.data;
            m_big_string.data     = new char[size];
            m_big_string.capacity = size;
            copy_memory(old_data, m_big_string.data, m_size);
            delete[] old_data;
        }
    }

private:
    union {
        struct {
            char*  data;
            size_t capacity;
        } m_big_string;

        char m_small_string[max_small_string_size + 1] {};
    };

    using BigString = decltype(m_big_string);

    size_t m_size { 0 };
};

class StringView {
public:
    StringView(const char* string) : m_string(string), m_size(String::length_of(string)) { }
    StringView(const String& string) : m_string(string.data()), m_size(string.size()) { }
    StringView(const char* string, size_t length) : m_string(string), m_size(length) { }

    const char* begin() const { return data(); }
    const char* end() const { return data() + m_size; }

    const char* data() const { return m_string; }

    size_t      size() const { return m_size; }

private:
    const char* m_string;
    size_t      m_size;
};

inline String::String(StringView view)
{
    construct_from(view.data(), view.size());
}
}
