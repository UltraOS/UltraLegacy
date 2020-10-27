#include "ClassicTheme.h"

namespace kernel {

// clang-format off
const u8 ClassicTheme::s_cursor_bitmap_data[] = {
    0b00000001, 0b00000000,
    0b00000011, 0b00000000,
    0b00000111, 0b00000000,
    0b00001111, 0b00000000,
    0b00011111, 0b00000000,
    0b00111111, 0b00000000,
    0b01111111, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b00000001,
    0b11111111, 0b00000011,
    0b11111111, 0b00000111,
    0b11111111, 0b00001111,
    0b01111111, 0b00000000,
    0b11110111, 0b00000000,
    0b11110011, 0b00000000,
    0b11100001, 0b00000001,
    0b11100000, 0b00000001,
    0b11000000, 0b00000000,
    0b00000000, 0b00000000
};

const u8 ClassicTheme::s_close_button_bitmap_data[] = {
    0b10000001,
    0b01000010,
    0b00100100,
    0b00011000,
    0b00011000,
    0b00100100,
    0b01000010,
    0b10000001,
};

const u8 ClassicTheme::s_maximize_button_bitmap_data[] = {
    0b00000000,
    0b00000000,
    0b00011000,
    0b00100100,
    0b01000010,
    0b10000001,
    0b00000000,
    0b00000000,
};

const u8 ClassicTheme::s_minimize_button_bitmap_data[] = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b11111111,
    0b00000000,
    0b00000000,
    0b00000000,
};

const Color ClassicTheme::s_palette[2] = { Color::transparent(), Color::white() };
// clang-format on

ClassicTheme::ClassicTheme()
    : m_cursor_bitmap(
        s_cursor_bitmap_data,
        cursor_bitmap_width,
        cursor_bitmap_height,
        Bitmap::Format::INDEXED_1_BPP,
        s_palette)
    , m_close_button_bitmap(
          s_close_button_bitmap_data,
          close_button_bitmap_width,
          close_button_bitmap_height,
          Bitmap::Format::INDEXED_1_BPP,
          s_palette)
    , m_maximize_button_bitmap(
          s_maximize_button_bitmap_data,
          maximize_button_bitmap_width,
          maximize_button_bitmap_height,
          Bitmap::Format::INDEXED_1_BPP,
          s_palette)
    , m_minimize_button_bitmap(
          s_minimize_button_bitmap_data,
          minimize_button_bitmap_width,
          minimize_button_bitmap_height,
          Bitmap::Format::INDEXED_1_BPP,
          s_palette)
{
}
}
