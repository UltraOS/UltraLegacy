#pragma once

#include "Core/Types.h"

namespace kernel {
    class PIC
    {
    public:
        static constexpr u8 master_port = 0x20;
        static constexpr u8 slave_port  = 0xA0;

        static constexpr u8 master_command = master_port;
        static constexpr u8 master_data    = master_port + 1;

        static constexpr u8 slave_command  = slave_port;
        static constexpr u8 slave_data     = slave_port + 1;

        static constexpr u8 end_of_interrupt_code = 0x20;

        static void end_of_interrupt(u8 request_number, bool spurious = false);

        static bool is_irq_being_serviced(u8 request_number);

        static void remap(u8 offset);
    };
}
