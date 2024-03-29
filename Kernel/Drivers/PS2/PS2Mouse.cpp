#include "PS2Mouse.h"
#include "Common/Logger.h"
#include "WindowManager/EventManager.h"

namespace kernel {

PS2Mouse::PS2Mouse(PS2Controller* parent, PS2Controller::Channel channel)
    : PS2Device(parent, channel)
{
    detect_subtype();
    set_resolution(default_resolution);
    enable_irq();
}

void PS2Mouse::set_resolution(u8 resolution)
{
    static constexpr u8 maximum_resolution = 0x03;

    ASSERT(resolution <= maximum_resolution);

    static constexpr u8 set_resolution_command = 0xE8;

    send_command(set_resolution_command);
    send_command(resolution);
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
        auto type = controller().identify_device(channel());
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

u8 PS2Mouse::bytes_in_packet() const
{
    switch (sub_type()) {
    case SubType::STANDARD_MOUSE:
        return 3;
    case SubType::SCROLL_WHEEL:
    case SubType::FIVE_BUTTONS:
        return 4;
    default: {
        String error_string;
        error_string << "PS2Mouse: Unknown sub type " << static_cast<u8>(sub_type());
        runtime::panic(error_string.data());
    }
    }
}

void PS2Mouse::append_packet_data(u8 data)
{
    static constexpr u8 always_1_bit = SET_BIT(3);

    if (m_packet_bytes == 0 && !(data & always_1_bit)) {
        warning() << "PS2Mouse: packet out of sync, resetting stream...";
        return;
    }

    m_packet[m_packet_bytes++] = data;

    if (m_packet_bytes == bytes_in_packet())
        parse_packet();
}

void PS2Mouse::parse_packet()
{
    static constexpr u8 y_overflow_bit = SET_BIT(7);
    static constexpr u8 x_overflow_bit = SET_BIT(6);

    static constexpr u8 y_sign_bit = SET_BIT(5);
    static constexpr u8 x_sign_bit = SET_BIT(4);
    static constexpr u8 mb_bit = SET_BIT(2);
    static constexpr u8 rb_bit = SET_BIT(1);
    static constexpr u8 lb_bit = SET_BIT(0);

    Action a {};

    bool is_y_negative = m_packet[0] & y_sign_bit;
    bool is_x_negative = m_packet[0] & x_sign_bit;

    a.middle_button_state = m_packet[0] & mb_bit ? VKState::PRESSED : VKState::RELEASED;
    a.right_button_state = m_packet[0] & rb_bit ? VKState::PRESSED : VKState::RELEASED;
    a.left_button_state = m_packet[0] & lb_bit ? VKState::PRESSED : VKState::RELEASED;

    a.x_delta = is_x_negative ? static_cast<i32>(0xFFFFFF00 | m_packet[1]) : m_packet[1];
    a.y_delta = is_y_negative ? static_cast<i32>(0xFFFFFF00 | m_packet[2]) : m_packet[2];

    if (m_packet[0] & x_overflow_bit)
        a.x_delta = 0;
    if (m_packet[0] & y_overflow_bit)
        a.y_delta = 0;

    if (sub_type() == SubType::SCROLL_WHEEL) {
        i32 z_delta = m_packet[3];

        if (m_packet[3] & SET_BIT(7))
            z_delta |= static_cast<i32>(0xFFFFFFF0);

        a.wheel_delta = static_cast<i8>(z_delta);
        a.scroll_direction = ScrollDirection::VERTICAL;

    } else if (sub_type() == SubType::FIVE_BUTTONS) {
        static constexpr u8 button_5_bit = SET_BIT(5);
        static constexpr u8 button_4_bit = SET_BIT(4);
        static constexpr u8 z_delta_mask = 0xF;

        switch (m_packet[3] & z_delta_mask) {
        case 0x1:
            a.wheel_delta = 1;
                a.scroll_direction = ScrollDirection::VERTICAL;
            break;
        case 0xF:
            a.wheel_delta = -1;
                a.scroll_direction = ScrollDirection::VERTICAL;
            break;
        case 0x2:
            a.wheel_delta = 1;
                a.scroll_direction = ScrollDirection::HORIZONTAL;
            break;
        case 0xE:
            a.wheel_delta = -1;
                a.scroll_direction = ScrollDirection::HORIZONTAL;
            break;
        }

        a.button_4_state = m_packet[3] & button_4_bit ? VKState::PRESSED : VKState::RELEASED;
        a.button_5_state = m_packet[3] & button_5_bit ? VKState::PRESSED : VKState::RELEASED;
    }

    EventManager::the().post_action(a);

    m_packet_bytes = 0;
}

bool PS2Mouse::handle_action()
{
    if (!data_available())
        return false;

    append_packet_data(read_data());
    return true;
}
}
