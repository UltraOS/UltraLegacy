#pragma once

#include "Drivers/Mouse.h"
#include "PS2Device.h"

namespace kernel {

class PS2Mouse : public PS2Device, public Mouse {
public:
    enum class SubType {
        STANDARD_MOUSE = 0x00,
        SCROLL_WHEEL   = 0x03,
        FIVE_BUTTONS   = 0x04,
    };

    PS2Mouse(PS2Controller::Channel channel);

    void set_sample_rate(u8 rate);

    void handle_action() override;

    StringView name() const override { return "PS2 Mouse"; }

    SubType sub_type() const { return m_sub_type; }

private:
    void detect_subtype();

    u8 bytes_in_packet() const;

    void append_packet_data(u8 data);
    void parse_packet();

private:
    u8 m_packet[4];
    u8 m_packet_bytes;

    SubType m_sub_type { SubType::STANDARD_MOUSE };
};
}
