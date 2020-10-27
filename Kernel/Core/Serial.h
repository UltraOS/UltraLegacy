#pragma once

#include "Common/String.h"
#include "IO.h"

namespace kernel {

class Serial {
    MAKE_STATIC(Serial);

public:
    enum class Port {
        COM1 = 0x3F8,
        COM2 = 0x2F8,
        COM3 = 0x3E8,
        COM4 = 0x2E8,
    };

    template <Port port>
    static void initialize()
    {
        static constexpr u16 raw_port = static_cast<u16>(port);

        static constexpr u16 interrupt_enable_register = raw_port + 1;
        static constexpr u16 fifo_control_register = raw_port + 2;
        static constexpr u16 line_control_register = raw_port + 3;
        static constexpr u16 modem_control_register = raw_port + 4;

        // Works with DLAB enabled
        static constexpr u16 divisor_low_register = raw_port;
        static constexpr u16 divisor_high_register = raw_port + 1;

        static constexpr u8 dlab_enable_bit = SET_BIT(7);

        // disable all serial IRQs
        IO::out8<interrupt_enable_register>(0x00);

        // Set the Baud rate to 115200 / 2
        IO::out8<line_control_register>(dlab_enable_bit);
        IO::out8<divisor_low_register>(0x02);
        IO::out8<divisor_high_register>(0x00);

        // 0b11 - 8 bits, 0b0 - 1 stop bit, 0b000 - no parity
        IO::out8<line_control_register>(0b11);

        // 14 byte IRQ threshold, clear both queues, enable FIFO
        IO::out8<fifo_control_register>(0b11000111);

        // aux output 2 enabled, rts/dtr set
        IO::out8<modem_control_register>(0b1011);
    }

    template <Port port>
    static bool is_ready_to_send()
    {
        static constexpr u16 line_status_register = static_cast<u16>(port) + 5;

        static constexpr u8 transmitter_holding_register_empty = SET_BIT(5);

        return IO::in8<line_status_register>() & transmitter_holding_register_empty;
    }

    template <Port port>
    static void write(char c)
    {
        static constexpr u16 data_register = static_cast<u16>(port);

        while (!is_ready_to_send<port>())
            ;

        IO::out8<data_register>(c);
    }

    template <Port port>
    static void write(StringView string)
    {
        for (char c : string)
            write<port>(c);
    }
};
}
