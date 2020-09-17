#include "Cursor.h"

namespace kernel {

// clang-format off
const u8 Cursor::s_bitmap_data[] = {
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
// clang-format on

const Color Cursor::s_palette[2] = { Color::transparent(), Color::white() };

Cursor::Cursor(Point location)
    : m_bitmap(s_bitmap_data, bitmap_width, bitmap_height, Bitmap::Format::INDEXED_1_BPP, s_palette),
      m_location(location)
{
}

}
