#include "PS2Mouse.h"
#include "Common/Logger.h"

namespace kernel {

PS2Mouse::PS2Mouse(PS2Controller::Channel channel) : PS2Device(channel)
{
    detect_subtype();
    enable_irq();
}

void PS2Mouse::set_sample_rate(u8 rate)
{
    static constexpr u8 set_sample_rate_command = 0xF3;

    send_command(set_sample_rate_command);
    send_command(rate);
}

void PS2Mouse::detect_subtype()
{
    auto current_type = [this]() -> SubType {
        auto type = PS2Controller::the().identify_device(channel());
        ASSERT(type.id_bytes == 1);

        return static_cast<SubType>(type.id[0]);
    };

    if (current_type() == SubType::STANDARD_MOUSE) {
        set_sample_rate(200);
        set_sample_rate(100);
        set_sample_rate(80);

        m_sub_type = current_type();
    }

    if (sub_type() == SubType::SCROLL_WHEEL) {
        set_sample_rate(200);
        set_sample_rate(200);
        set_sample_rate(80);

        m_sub_type = current_type();
    }

    log() << "PS2Mouse: detected sub type " << static_cast<u8>(m_sub_type);
}

void PS2Mouse::handle_action()
{
    while (PS2Controller::the().status().output_full) {
        log() << "MouseMoved: " << format::as_hex << read_data();
    }
}
}
