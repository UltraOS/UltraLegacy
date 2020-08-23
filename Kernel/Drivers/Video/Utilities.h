#pragma once

#include "Common/Types.h"
#include "Common/Pair.h"

namespace kernel {

// represents an RGBA value in little endian layout
struct RGBA {
    u8 b;
    u8 g;
    u8 r;
    u8 a;

    static constexpr RGBA white() { return { 0xFF, 0xFF, 0xFF, 0xFF }; }
    static constexpr RGBA black() { return { 0x00, 0x00, 0x00, 0xFF }; }

    u32 as_u32() { return *reinterpret_cast<u32*>(this); }
};

class Rect {
public:
    Rect(size_t top_left_x, size_t top_left_y, size_t width, size_t height)
        : m_x(top_left_x), m_y(top_left_y), m_width(width), m_height(height) {}

    Pair<size_t, size_t> top_left() const  { return { m_x, m_y }; }
    Pair<size_t, size_t> bottom_right() const { return { m_x + m_width, m_y + m_height }; }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

private:
    size_t m_x;
    size_t m_y;
    size_t m_width;
    size_t m_height;
};
}
