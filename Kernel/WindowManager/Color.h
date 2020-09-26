#pragma once

#include "Common/Types.h"

namespace kernel {

// represents an RGBA value in little endian layout (BGRA)
class Color {
public:
    constexpr Color(u8 r, u8 g, u8 b, u8 a = 0xFF) : m_b(b), m_g(g), m_r(r), m_a(a) { }

    static constexpr Color white() { return { 0xFF, 0xFF, 0xFF }; }
    static constexpr Color black() { return { 0x00, 0x00, 0x00 }; }

    static constexpr Color red() { return { 0xFF, 0x00, 0x00 }; }
    static constexpr Color green() { return { 0x00, 0xFF, 0x00 }; }
    static constexpr Color blue() { return { 0x00, 0x00, 0xFF }; }
    static constexpr Color transparent() { return { 0x00, 0x00, 0x00, 0x00 }; }

    constexpr u8 r() const { return m_r; }
    constexpr u8 g() const { return m_g; }
    constexpr u8 b() const { return m_b; }
    constexpr u8 a() const { return m_a; }

    u32 as_u32() const { return *reinterpret_cast<const u32*>(this); }

private:
    u8 m_b;
    u8 m_g;
    u8 m_r;
    u8 m_a;
};
}
