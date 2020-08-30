#pragma once

#include "Common/Pair.h"
#include "Common/Types.h"

namespace kernel {

class Point : public Pair<size_t, size_t> {
public:
    Point() = default;
    Point(size_t x, size_t y) : Pair(x, y) { }

    size_t x() const { return left(); }
    size_t y() const { return right(); }
};

// represents an RGBA value in little endian layout (BGRA)
class Color {
public:
    constexpr Color(u8 r, u8 g, u8 b, u8 a = 0xFF) : m_b(b), m_g(g), m_r(r), m_a(a) { }

    static constexpr Color white() { return { 0xFF, 0xFF, 0xFF }; }
    static constexpr Color black() { return { 0x00, 0x00, 0x00 }; }

    static constexpr Color red() { return { 0xFF, 0x00, 0x00 }; }
    static constexpr Color green() { return { 0x00, 0xFF, 0x00 }; }
    static constexpr Color blue() { return { 0x00, 0x00, 0xFF }; }

    constexpr u8 r() const { return m_r; }
    constexpr u8 g() const { return m_g; }
    constexpr u8 b() const { return m_b; }
    constexpr u8 a() const { return m_a; }

    u32 as_u32() { return *reinterpret_cast<u32*>(this); }

private:
    u8 m_b;
    u8 m_g;
    u8 m_r;
    u8 m_a;
};

class Rect {
public:
    Rect() = default;

    Rect(size_t top_left_x, size_t top_left_y, size_t width, size_t height)
        : m_x(top_left_x), m_y(top_left_y), m_width(width), m_height(height)
    {
    }

    void set_top_left_x(size_t x) { m_x = x; }
    void set_top_left_y(size_t y) { m_y = y; }

    void set_width(size_t width) { m_width = width; }
    void set_height(size_t height) { m_height = height; }

    Point top_left() const { return { m_x, m_y }; }
    Point bottom_right() const { return { m_x + m_width, m_y + m_height }; }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

    Point center() const { return { m_x + m_width / 2, m_y + m_height / 2 }; }

private:
    size_t m_x { 0 };
    size_t m_y { 0 };
    size_t m_width { 0 };
    size_t m_height { 0 };
};
}
