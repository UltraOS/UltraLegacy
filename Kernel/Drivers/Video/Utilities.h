#pragma once

#include "Common/Types.h"

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
}
