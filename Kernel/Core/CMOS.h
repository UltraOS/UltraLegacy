#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "IO.h"

namespace kernel {

class CMOS {
    MAKE_STATIC(CMOS)
public:
    static constexpr u8 selector_port = 0x70;
    static constexpr u8 register_port = 0x71;

    template <u8 register_index>
    static u8 read()
    {
        IO::out8<selector_port>(register_index);
        return IO::in8<register_port>();
    }

    template <u8 register_index>
    static void write(u8 value)
    {
        IO::out8<selector_port>(register_index);
        IO::out8<register_index>(value);
    }
};
}
