#pragma once

#include "Types.h"
#include "Traits.h"

template<typename T>
enable_if_t<is_integral<T>::value> to_string_buffer(T number, char* string, size_t max_size, bool& ok, bool null_terminate = true)
{
    bool is_negative = false;
    ok = true;
    size_t required_size = 0;

    if (number == 0)
    {
        if (max_size >= (null_terminate ? 2 : 1))
        {
            string[0] = '0';
            if (null_terminate)
                string[1] = '\0';
            return;
        }

        ok = false;
        return;
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
    {
        ok = false;
        return;
    }

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
}
