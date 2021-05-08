#pragma once

#include "Common/Conversions.h"
#include "Common/Memory.h"
#include "Common/Optional.h"
#include "Common/Utilities.h"
#include "Memory/Range.h"

namespace kernel {

constexpr inline size_t length_of(const char* ptr)
{
    ASSERT(ptr != nullptr);

    size_t length = 0;

    while (*(ptr++))
        ++length;

    return length;
}

inline bool is_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}

inline bool is_lower(char c)
{
    return c >= 'a' && c <= 'z';
}

inline char to_lower(char c)
{
    static constexpr i32 offset_to_lower = 'a' - 'A';
    static_assert(offset_to_lower > 0, "Negative lower to upper offset");

    if (is_upper(c))
        return c + offset_to_lower;

    return c;
}

inline void to_lower(char* str, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        str[i] = to_lower(str[i]);
}

inline char to_upper(char c)
{
    static constexpr i32 offset_to_upper = 'a' - 'A';
    static_assert(offset_to_upper > 0, "Negative lower to upper offset");

    if (is_lower(c))
        return c - offset_to_upper;

    return c;
}

inline void to_upper(char* str, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        str[i] = to_upper(str[i]);
}

template <size_t N>
inline void to_lower(char (&array)[N])
{
    to_lower(array, N);
}

template <size_t N>
inline void to_upper(char (&array)[N])
{
    to_upper(array, N);
}

class StringView;

bool are_equal(StringView, StringView);
template <typename LStrType, typename RStrType>
bool are_equal(const LStrType&, const RStrType&);

bool are_case_insensitive_equal(StringView, StringView);
template <typename LStrType, typename RStrType>
bool are_case_insensitive_equal(const LStrType&, const RStrType&);

bool is_less(StringView, StringView);
template <typename LStrType, typename RStrType>
bool is_less(const LStrType&, const RStrType&);

bool starts_with(StringView, StringView);
template <typename LStrType, typename RStrType>
bool starts_with(const LStrType&, const RStrType&);

Optional<size_t> find(StringView, StringView);
template <typename LStrType, typename RStrType>
Optional<size_t> find(const LStrType&, const RStrType&);

Optional<size_t> find_last(StringView, StringView);
template <typename LStrType, typename RStrType>
Optional<size_t> find_last(const LStrType&, const RStrType&);

template <typename Self>
class FindableComparable {
public:
    template <typename T>
    [[nodiscard]] bool equals(const T& other) const
    {
        return ::kernel::are_equal(static_cast<const Self&>(*this), other);
    }

    template <typename T>
    [[nodiscard]] bool case_insensitive_equals(const T& other) const
    {
        return ::kernel::are_case_insensitive_equal(static_cast<const Self&>(*this), other);
    }

    template <typename T>
    [[nodiscard]] bool operator==(const T& rhs) const { return are_equal(static_cast<const Self&>(*this), rhs); }
    template <typename T>
    [[nodiscard]] bool operator!=(const T& rhs) const { return !are_equal(static_cast<const Self&>(*this), rhs); }

    [[nodiscard]] bool is_less_than(const Self& other)
    {
        return ::kernel::is_less(static_cast<const Self&>(*this), other);
    }

    template <typename T>
    bool operator<(const T& rhs) const { return is_less(static_cast<const Self&>(*this), rhs); }

    template <typename T>
    [[nodiscard]] bool starts_with(const T& other) const
    {
        return ::kernel::starts_with(static_cast<const Self&>(*this), other);
    }

    template <typename T>
    [[nodiscard]] Optional<size_t> find(const T& other) const
    {
        return ::kernel::find(static_cast<const Self&>(*this), other);
    }

    template <typename T>
    [[nodiscard]] Optional<size_t> find_last(const T& other) const
    {
        return ::kernel::find_last(static_cast<const Self&>(*this), other);
    }

    [[nodiscard]] Address find_in_range(const Range& range, size_t step = 1) const;

private:
    [[nodiscard]] const char* data() const { return static_cast<const Self*>(this)->data(); }
    [[nodiscard]] size_t size() const { return static_cast<const Self*>(this)->size(); }
};

class StringView : public FindableComparable<StringView> {
public:
    constexpr StringView() = default;

    constexpr StringView(const char* string)
        : m_string(string)
        , m_size(length_of(string))
    {
    }

    constexpr StringView(const char* begin, const char* end)
        : m_string(begin)
        , m_size(end - begin)
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

    template <size_t N>
    static constexpr StringView from_char_array(char (&array)[N])
    {
        return StringView(array, N);
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

    [[nodiscard]] const char& operator[](size_t i) const { return at(i); }

private:
    const char* m_string { nullptr };
    size_t m_size { 0 };
};

inline constexpr StringView operator""_sv(const char* string, size_t size)
{
    return { string, size };
}

class String;

enum class format {
    as_dec,
    as_hex,
};

template <typename Self>
class StreamingFormatter {
public:
    // clang-format off
    template <typename T>
    using enable_if_default_serializable_t = typename
        enable_if<is_same_v<T, Address32>   ||
                  is_same_v<T, Address64>   ||
                  is_same_v<T, Address>     ||
                  is_integral_v<T>          ||
                  is_pointer_v<T>           ||
                  is_same_v<T, const char*> ||
                  is_same_v<T, StringView>  ||
                  is_same_v<T, format>      ||
                  is_same_v<T, bool>, Self&>::type;

    template <typename T>
    using enable_if_default_ref_serializable_t = typename
        enable_if<is_same_v<T, String>, Self&>::type;
    // clang-format on

    void set_format(format new_format)
    {
        m_format = new_format;
    }

    void append(const String& string);

    void append(StringView string)
    {
        static_cast<Self*>(this)->write(string);
    }

    void append(const char* string)
    {
        append(StringView(string));
    }

    void append(char c)
    {
        append(StringView(&c, 1));
    }

    template <typename T>
    enable_if_t<is_arithmetic_v<T>> append(T number)
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

    void append(bool value)
    {
        static constexpr StringView true_value = "true"_sv;
        static constexpr StringView false_value = "false"_sv;

        append(value ? true_value : false_value);
    }

    template <typename T>
    enable_if_default_serializable_t<T> operator<<(T value)
    {
        if constexpr (is_same_v<T, format>) {
            set_format(value);
        } else {
            append(value);
        }

        return static_cast<Self&>(*this);
    }

    template <typename T>
    enable_if_default_ref_serializable_t<T> operator<<(const T& value)
    {
        append(value);

        return static_cast<Self&>(*this);
    }

    template <typename T>
    enable_if_default_serializable_t<T> operator+=(T value)
    {
        append(value);
        return static_cast<Self&>(*this);
    }

    template <typename T>
    enable_if_default_ref_serializable_t<T> operator+=(const T& value)
    {
        append(value);
        return static_cast<Self&>(*this);
    }

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

private:
    format m_format { format::as_dec };
};

class String : public StreamingFormatter<String>, public FindableComparable<String> {
public:
    String()
    {
        reset_small_string();
    }

    String(const String& other)
    {
        reset_small_string();

        auto size_to_copy = other.size();
        if (size_to_copy == 0)
            return;

        reserve(size_to_copy);
        copy_memory(other.data(), data(), size_to_copy + 1);

        if (is_small())
            m_str.as_small.set_size(size_to_copy);
        else
            m_str.as_big.size = size_to_copy;
    }

    String(String&& other)
    {
        if (other.is_small()) {
            copy_memory(&other.m_str.as_small, &m_str.as_small, sizeof(SmallString));
            other.reset_small_string();
            return;
        }

        m_str.as_big = other.m_str.as_big;
        other.reset_small_string();
    }

    String(const char* string)
    {
        reset_small_string();
        append(string);
    }

    explicit String(StringView string)
    {
        reset_small_string();
        append(string);
    }

    String& operator=(const String& other)
    {
        if (this == &other)
            return *this;

        auto size_to_copy = other.size();
        if (size_to_copy == 0) {
            clear();
            return *this;
        }

        reserve(size_to_copy);
        copy_memory(other.data(), data(), size_to_copy + 1);

        if (is_small())
            m_str.as_small.set_size(size_to_copy);
        else
            m_str.as_big.size = size_to_copy;

        return *this;
    }

    String& operator=(String&& other)
    {
        if (this == &other)
            return *this;

        size_t other_size = other.size();

        if (other_size == 0) {
            clear();
            return *this;
        }

        bool is_this_small = is_small();

        // Slow path, since other is small we cannot just steal its data pointer.
        if (other.is_small()) {
            if (is_this_small) {
                copy_memory(&other.m_str.as_small, &m_str.as_small, sizeof(SmallString));
                return *this;
            }

            auto& this_str = m_str.as_big;
            copy_memory(other.m_str.as_small.characters, this_str.characters, other_size + 1);
            this_str.size = other_size;
            return *this;
        }

        if (!is_this_small)
            delete[] m_str.as_big.characters;

        m_str.as_big = other.m_str.as_big;
        other.reset_small_string();

        return *this;
    }

    String& operator=(const char* string)
    {
        clear();
        append(string);

        return *this;
    }

    String& operator=(StringView string)
    {
        clear();
        append(string);

        return *this;
    }

    char& at(size_t index)
    {
        ASSERT(index < size());
        return data()[index];
    }

    const char& at(size_t index) const
    {
        ASSERT(index < size());
        return data()[index];
    }

    char& operator[](size_t index)
    {
        return at(index);
    }

    const char& operator[](size_t index) const
    {
        return at(index);
    }

    void reserve(size_t character_count)
    {
        // align up to 2
        if (character_count & 1)
            character_count += 1;

        auto current_capacity = capacity();

        if (current_capacity >= character_count)
            return;

        // force at least x2
        character_count = max(character_count, current_capacity * 2);

        char* new_characters = new char[character_count + 1];

        if (is_small()) {
            auto& str = m_str.as_small;
            size_t size = str.size();

            copy_memory(str.characters, new_characters, size + 1);
            m_str.as_big = { character_count, size, new_characters };
            return;
        }

        auto& str = m_str.as_big;
        copy_memory(str.characters, new_characters, str.size + 1);
        delete[] str.characters;
        str.characters = new_characters;
    }

    void write(StringView string)
    {
        if (string.empty())
            return;

        auto original_size = size();
        auto new_size = original_size + string.size();

        reserve(new_size);

        char* buffer_ptr = nullptr;

        if (is_small()) {
            auto& str = m_str.as_small;
            buffer_ptr = str.characters;
            str.set_size(new_size);
        } else {
            auto& str = m_str.as_big;
            buffer_ptr = str.characters;
            str.size = new_size;
        }

        buffer_ptr += original_size;
        copy_memory(string.data(), buffer_ptr, string.size());
        buffer_ptr[string.size()] = '\0';
    }

    void strip(char c = ' ')
    {
        auto* str = data();
        auto sz = size();

        if (sz == 0)
            return;

        size_t i = 0;
        while (i < sz && str[i] == c)
            i++;

        if (i == sz) {
            clear();
            return;
        }

        size_t j = sz - 1;
        while (j > i && str[j] == c)
            j--;

        if (i == 0 && j == sz - 1)
            return;

        if (i)
            move_memory(str + i, str, sz - i);

        auto new_size = j - i + 1;

        if (is_small()) {
            auto& small = m_str.as_small;
            small.characters[new_size] = '\0';
            small.set_size(new_size);
            return;
        }

        auto& big = m_str.as_big;
        big.characters[new_size] = '\0';
        big.size = new_size;
    }

    void rstrip(char c = ' ')
    {
        auto* str = data();
        auto sz = size();

        if (sz == 0)
            return;

        size_t i = sz;
        while (i-- > 0) {
            if (str[i] == c)
                continue;

            break;
        }

        if (i == sz - 1)
            return;

        if (i > sz) {
            clear();
            return;
        }

        auto new_size = i + 1;

        if (is_small()) {
            auto& small = m_str.as_small;
            small.characters[new_size] = '\0';
            small.set_size(new_size);
            return;
        }

        auto& big = m_str.as_big;
        big.characters[new_size] = '\0';
        big.size = new_size;
    }

    void pop_back()
    {
        if (is_small()) {
            auto& str = m_str.as_small;
            auto original_size = str.size();
            ASSERT(original_size != 0);
            str.set_size(original_size - 1);
            str.characters[original_size - 1] = '\0';

            return;
        }

        auto& str = m_str.as_big;
        ASSERT(str.size != 0);
        str.size--;
        str.characters[str.size] = '\0';
    }

    [[nodiscard]] size_t size() const { return is_small() ? m_str.as_small.size() : m_str.as_big.size; }
    [[nodiscard]] bool empty() const { return size() == 0; }

    [[nodiscard]] size_t capacity() const { return is_small() ? SmallString::capacity : m_str.as_big.capacity; }

    [[nodiscard]] char* data() { return is_small() ? m_str.as_small.characters : m_str.as_big.characters; }
    [[nodiscard]] const char* data() const { return is_small() ? m_str.as_small.characters : m_str.as_big.characters; }
    [[nodiscard]] const char* c_string() const { return data(); }

    [[nodiscard]] char* begin() { return data(); }
    [[nodiscard]] char* end() { return data() + size(); }

    [[nodiscard]] StringView to_view() const { return { data(), size() }; }
    explicit operator StringView() const { return to_view(); }

    void to_lower()
    {
        if (empty())
            return;

        ::kernel::to_lower(data(), size());
    }

    void to_upper()
    {
        if (empty())
            return;

        ::kernel::to_upper(data(), size());
    }

    void clear()
    {
        if (is_small()) {
            reset_small_string();
            return;
        }

        m_str.as_big.size = 0;
        m_str.as_big.characters[0] = '\0';
    }

    ~String()
    {
        if (is_small())
            return;

        delete[] m_str.as_big.characters;
    }

private:
    struct BigString {
        size_t capacity;
        size_t size;
        char* characters;
    };

    // meta layout:
    // bit 0 - small mode enable bit
    // bit 1...7 - size of the short string
    // (inspired by clang's libc++)
    struct SmallString {
        static constexpr u8 mode_bit = 1;
        static constexpr u8 size_shift = 1;
        static constexpr size_t capacity = sizeof(BigString) - 2; // meta byte + null character

        u8 meta;
        char characters[capacity + 1];

        void set_size(size_t size) { meta = (size << size_shift) | mode_bit; }

        [[nodiscard]] size_t size() const { return meta >> size_shift; }
    };

    static_assert(sizeof(BigString) == sizeof(SmallString));

    [[nodiscard]] bool is_small() const
    {
        u8 meta_byte = 0;
        copy_memory(&m_str, &meta_byte, sizeof(meta_byte));

        return meta_byte & SmallString::mode_bit;
    }

    void reset_small_string()
    {
        zero_memory(&m_str.as_small, sizeof(SmallString));
        m_str.as_small.set_size(0);
    }

private:
    union {
        SmallString as_small;
        BigString as_big;
    } m_str {};
};

template <typename Self>
inline void StreamingFormatter<Self>::append(const String& string)
{
    append(string.to_view());
}

template <size_t buffer_size = 256>
class StackString : public StreamingFormatter<StackString<buffer_size>>, public FindableComparable<StackString<buffer_size>> {
public:
    static_assert(buffer_size > 1, "Buffer size cannot be smaller than 2");

    [[nodiscard]] StringView to_view() const { return StringView(m_buffer, m_current_offset); }

    [[nodiscard]] char* data() { return m_buffer; }
    [[nodiscard]] const char* data() const { return m_buffer; }

    [[nodiscard]] size_t capacity() const { return buffer_size; }
    [[nodiscard]] size_t size() const { return m_current_offset; }

    [[nodiscard]] size_t size_left() const { return buffer_size - m_current_offset - 1; }

    void write(StringView string)
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

template <size_t Size>
bool operator==(const char (&l)[Size], const StringView& r)
{
    return r.equals(StringView(l, Size));
}

template <size_t Size>
bool operator!=(const char (&l)[Size], const StringView& r)
{
    return !r.equals(StringView(l, Size));
}

template <typename LStrType, typename RStrType>
bool are_equal(const LStrType& lhs, const RStrType& rhs)
{
    return are_equal(StringView(lhs), StringView(rhs));
}

template <typename LStrType, typename RStrType>
bool are_case_insensitive_equal(const LStrType& lhs, const RStrType& rhs)
{
    return are_case_insensitive_equal(StringView(lhs), StringView(rhs));
}

template <typename LStrType, typename RStrType>
bool is_less(const LStrType& lhs, const RStrType& rhs)
{
    return is_less(StringView(lhs), StringView(rhs));
}

inline bool are_equal(StringView lhs, StringView rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i] != rhs[i])
            return false;
    }

    return true;
}

inline bool are_case_insensitive_equal(StringView lhs, StringView rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < lhs.size(); ++i) {
        if (to_lower(lhs[i]) == to_lower(rhs[i]))
            continue;

        return false;
    }

    return true;
}

inline bool is_less(StringView lhs, StringView rhs)
{
    size_t i = 0;
    size_t j = 0;

    while (i < lhs.size() && j < rhs.size()) {
        if (lhs[i] < rhs[j])
            return true;
        if (rhs[j] < lhs[i])
            return false;

        ++i;
        ++j;
    }

    return (i == lhs.size()) && (j != rhs.size());
}

inline bool starts_with(StringView lhs, StringView rhs)
{
    if (rhs.size() > lhs.size())
        return false;
    if (rhs.empty())
        return true;

    for (size_t i = 0; i < rhs.size(); ++i) {
        if (lhs.at(i) != rhs.at(i))
            return false;
    }

    return true;
}

inline Optional<size_t> find(StringView lhs, StringView rhs)
{
    if (rhs.size() > lhs.size())
        return {};
    if (rhs.empty())
        return 0;

    for (size_t i = 0; i < lhs.size() - rhs.size() + 1; ++i) {
        if (lhs[i] != rhs[0])
            continue;

        size_t j = i;
        size_t k = 0;

        while (k < rhs.size()) {
            if (lhs[j++] != rhs[k])
                break;

            k++;
        }

        if (k == rhs.size())
            return i;
    }

    return {};
}

template <typename LStrType, typename RStrType>
Optional<size_t> find(const LStrType& lhs, const RStrType& rhs)
{
    return find(StringView(lhs), StringView(rhs));
}

inline Optional<size_t> find_last(StringView lhs, StringView rhs)
{
    if (rhs.size() > lhs.size())
        return {};
    if (rhs.empty())
        return lhs.size();

    for (size_t i = (lhs.size() - 1); i >= (rhs.size() - 1); i--) {
        if (i > lhs.size())
            return {};

        if (lhs[i] != rhs[rhs.size() - 1])
            continue;

        size_t j = i;
        size_t k = rhs.size() - 1;

        while (k < rhs.size()) {
            if (lhs[j--] != rhs[k])
                break;

            k--;
        }

        if (k > rhs.size())
            return i - (rhs.size() - 1);
    }

    return {};
}

template <typename LStrType, typename RStrType>
Optional<size_t> find_last(const LStrType& lhs, const RStrType& rhs)
{
    return find_last(StringView(lhs), StringView(rhs));
}

inline Address find_in_range(const Range& range, size_t step, StringView string)
{
    for (auto pointer = range.begin(); pointer + string.size() < range.end(); pointer += step) {
        if (StringView(Address(pointer).as_pointer<const char>(), string.size()).equals(string))
            return pointer;
    }

    return {};
}

template <typename Self>
Address FindableComparable<Self>::find_in_range(const Range& range, size_t step) const
{
    return ::kernel::find_in_range(range, step, StringView(static_cast<const Self&>(*this)));
}

inline bool operator==(const char* lhs, StringView rhs) { return StringView(lhs) == rhs; }
inline bool operator==(const char* lhs, const String& rhs) { return StringView(lhs) == rhs; }
inline bool operator!=(const char* lhs, StringView rhs) { return StringView(lhs) != rhs; }
inline bool operator!=(const char* lhs, const String& rhs) { return StringView(lhs) != rhs; }
inline bool operator<(const char* lhs, StringView rhs) { return StringView(lhs) < rhs; }
inline bool operator<(const char* lhs, const String& rhs) { return StringView(lhs) < rhs; }

}