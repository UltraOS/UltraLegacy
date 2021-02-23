#pragma once

#include "Drivers/Mouse.h"
#include "PS2Device.h"

namespace kernel {

class PS2Mouse final : public PS2Device, public Mouse {
public:
    enum class SubType {
        STANDARD_MOUSE = 0x00,
        SCROLL_WHEEL = 0x03,
        FIVE_BUTTONS = 0x04,
    };

    static constexpr u8 default_resolution = 0x1;

    explicit PS2Mouse(PS2Controller::Channel channel);

    void set_resolution(u8);
    void set_sample_rate(u8);

    [[nodiscard]] StringView device_name() const override { return "PS2 Mouse"; }
    [[nodiscard]] SubType sub_type() const { return m_sub_type; }

private:
    void detect_subtype();
    void handle_action() override;

    [[nodiscard]] u8 bytes_in_packet() const;

    void append_packet_data(u8 data);
    void parse_packet();

private:
    u8 m_packet[4] {};
    u8 m_packet_bytes {};

    SubType m_sub_type { SubType::STANDARD_MOUSE };
};
}
