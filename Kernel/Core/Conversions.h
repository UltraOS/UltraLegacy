#pragma once

#include "Types.h"
#include "Traits.h"
#include "Macros.h"

template<typename T>
enable_if_t<is_integral_v<T>, bool> to_string(T number, char* string, size_t max_size, bool null_terminate = true)
{
    bool is_negative = false;
    size_t required_size = 0;

    if (number == 0)
    {
        if (max_size >= (null_terminate ? 2 : 1))
        {
            string[0] = '0';
            if (null_terminate)
                string[1] = '\0';
            return true;
        }

        return false;
    }
    else if (number < 0)
    {
        is_negative = true;
        number = -number;
        ++required_size;
    }

    T copy = number;

    while (copy)
    {
        copy /= 10;
        ++required_size;
    }

    if (required_size + !!null_terminate > max_size)
        return false;

    T modulos = 0;
    for (size_t divisor = 0; divisor < required_size - !!is_negative; ++divisor)
    {
        modulos = number % 10;
        number /= 10;
        string[required_size - (divisor + 1)] = static_cast<char>(modulos + '0');
    }

    if (is_negative)
        string[0] = '-';

    if (null_terminate)
        string[required_size] = '\0';

    return true;
}

template<typename T>
enable_if_t<is_floating_point_v<T>, bool> to_string(T number, char* string, size_t max_size, bool null_terminate = true)
{
    number += 0.005; // to reduce the digit count

    i64 as_int = number;
    i64 remainder = static_cast<i64>(number * 100) % 100;

    size_t required_size = 0;
    i64 copy = as_int;

    while (copy) {
        copy /= 10;
        ++required_size;
    }

    size_t i = 0;
    while (as_int > 0 && max_size--) {
        string[required_size - i - 1] = (as_int % 10) + '0';
        as_int /= 10;
        ++i;
    }

    if (max_size < 3 + !!null_terminate)
        return false;

    string[i++] = '.';
    string[i++] = (remainder / 10) + '0';
    string[i++] = (remainder % 10) + '0';

    if (null_terminate)
        string[i] = '\0';

    return true;
}

template<typename T>
enable_if_t<is_integral_v<T>, bool> to_hex_string(T number, char* string, size_t max_size, bool null_terminate = true)
{
    constexpr auto required_length = sizeof(number) * 2 + 2; // '0x' + 2 chars per hex byte

    if (max_size < required_length + !!null_terminate)
        return false;

    string[0] = '0';
    string[1] = 'x';
    string[required_length] = '\0';

    static constexpr auto hex_digits = "0123456789ABCDEF";

    size_t j = 0;
    for (size_t i = 0; j < sizeof(number) * 2 * 4; ++i)
    {
        string[required_length - i - 1] = hex_digits[(number >> j) & 0x0F];
        j += 4;
    }

    return true;
}

template<typename T>
enable_if_t<is_floating_point_v<T>, bool> to_hex_string(T number, char* string, size_t max_size, bool null_terminate = true)
{
    return to_hex_string(static_cast<i64>(number), string, max_size, null_terminate);
}

template<typename T>
enable_if_t<is_integral_v<T>, T> bytes_to_megabytes(T bytes)
{
    return bytes / MB;
}

template<typename T>
enable_if_t<is_arithmetic_v<T>, float> bytes_to_megabytes_precise(T bytes)
{
    return bytes / static_cast<float>(MB);
}
