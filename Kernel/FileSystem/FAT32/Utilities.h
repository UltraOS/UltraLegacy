#pragma once

#include "Common/String.h"

namespace kernel {

static constexpr size_t short_name_length = 8;
static constexpr size_t short_extension_length = 3;
static constexpr u8 max_sequence_number = 20;

Pair<size_t, size_t> length_of_name_and_extension(StringView file_name)
{
    auto last_dot = file_name.find_last("."_sv);

    auto name_length = last_dot.value_or(file_name.size());
    size_t extension_length = 0;

    if (name_length == 0)
        extension_length = file_name.size();
    else if (name_length < file_name.size())
        extension_length = file_name.size() - name_length - 1;

    return { name_length, extension_length };
}

String generate_short_name(StringView long_name)
{
    String short_name;

    auto [name_length, extension_length] = length_of_name_and_extension(long_name);

    bool needs_numeric_tail = false;

    if (name_length > short_name_length || extension_length > short_extension_length)
        needs_numeric_tail = true;

    auto name_chars_to_copy = min(needs_numeric_tail ? short_name_length - 2 : short_name_length, name_length);

    size_t name_chars_copied = 0;
    for (size_t i = 0; i < name_length; ++i) {
        if (long_name[i] == '.' || long_name[i] == ' ')
            continue;

        short_name.append(long_name[i]);
        ++name_chars_copied;

        if (name_chars_to_copy == name_chars_copied)
            break;
    }

    if (needs_numeric_tail) {
        short_name.append('~');
        short_name.append('1');
        name_chars_copied += 2;
    }

    auto padding_chars = short_name_length - name_chars_copied;
    while (padding_chars--)
        short_name.append(' ');

    auto extension_chars = min(short_extension_length, extension_length);
    padding_chars = short_extension_length - extension_chars;

    if (extension_chars) {
        size_t extension_index = name_length + 1;
        size_t last = extension_index + extension_chars;

        for (size_t i = extension_index; i < last; ++i)
            short_name.append(long_name[i]);
    }

    while (padding_chars--)
        short_name.append(' ');

    short_name.to_upper();
    return short_name;
}

String next_short_name(StringView current, bool& ok)
{
    if (StringView(current.data() + 1, 7) == "~999999"_sv) {
        ok = false;
        return {};
    }

    ok = true;

    String new_short_name(current);

    auto numeric_tail_pos = current.find_last("~"_sv);
    auto end_of_name = current.find(" "_sv);

    if (!numeric_tail_pos) {
        auto new_tail_pos = min(short_name_length - 2, end_of_name.value_or(short_name_length - 2));

        new_short_name[new_tail_pos + 0] = '~';
        new_short_name[new_tail_pos + 1] = '1';
        return new_short_name;
    }

    auto end_of_numeric_tail = min(end_of_name.value_or(short_name_length + 1), short_name_length);

    size_t number = 0;
    bool would_overflow = true;

    for (size_t i = numeric_tail_pos.value() + 1; i < end_of_numeric_tail; ++i) {
        number *= 10;
        number += new_short_name[i] - '0';
        would_overflow &= (new_short_name[i] == '9');
    }

    number++;

    if (!would_overflow) {
        for (size_t i = end_of_numeric_tail - 1; i > numeric_tail_pos.value(); --i) {
            new_short_name[i] = static_cast<char>(number % 10) + '0';
            number /= 10;
        }

        return new_short_name;
    }

    bool can_grow_downwards = end_of_numeric_tail != short_name_length;

    size_t new_start = can_grow_downwards ? numeric_tail_pos.value() : numeric_tail_pos.value() - 1;
    size_t new_end = can_grow_downwards ? end_of_numeric_tail + 1 : end_of_numeric_tail;

    for (size_t i = new_end - 1; i > new_start; --i) {
        new_short_name[i] = static_cast<char>(number % 10) + '0';
        number /= 10;
    }

    new_short_name[new_start] = '~';
    return new_short_name;
}

u8 generate_short_name_checksum(StringView name)
{
    u8 sum = 0;
    const u8* byte_ptr = reinterpret_cast<const u8*>(name.data());

    for (size_t i = short_name_length + short_extension_length; i != 0; i--) {
        sum = (sum >> 1) + ((sum & 1) << 7);
        sum += *byte_ptr++;
    }

    return sum;
}

}