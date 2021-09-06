#include "SynapticsTouchpad.h"
#include "Interrupts/Timer.h"
#include "WindowManager/EventManager.h"

#define SYN_LOG log("SynapticsTouchpad")
#define SYN_WARN warning("SynapticsTouchpad")

#define SYN_DEBUG_MODE

#ifdef SYN_DEBUG_MODE
#define SYN_DEBUG log("SynapticsTouchpad")
#else
#define SYN_DEBUG DummyLogger()
#endif

namespace kernel {

SynapticsTouchpad::SynapticsTouchpad(PS2Controller* parent, PS2Controller::Channel channel, const IdentificationPage& id)
    : PS2Device(parent, channel)
{
    SYN_DEBUG << "version " << id.major_version << "." << id.minor_version;

    auto res = query<ResolutionPage>(8);
    if (IS_BIT_SET(res.highest_bit_must_be_set, 7) && res.x_units_per_mm && res.y_units_per_mm) {
        SYN_DEBUG << "x/mm " << res.x_units_per_mm << " y/mm " << res.y_units_per_mm;
        m_delta_per_pixel_x = res.x_units_per_mm / 8;
        m_delta_per_pixel_y = res.y_units_per_mm / 8;
    } else {
        SYN_WARN << "resolution query returned bogus data, setting defaults";
        m_delta_per_pixel_x = 40 / 8;
        m_delta_per_pixel_y = 64 / 8;
    }

    Mode m {};
    m.absolute = 1;
    m.rate = 1;

    auto caps = query<CapabilitiesPage>(2);

    m_supports_w = caps.has_extended_caps;

    if (!m_supports_w) {
        SYN_DEBUG << "no extended capabilities";
        set_mode(m);
        return;
    }

    m.wmode = 1;

    auto max_page = 8 + caps.max_query_page;
    SYN_DEBUG << "max query page is " << max_page;

    String supported_caps;
    supported_caps.reserve(64);
    if (caps.middle_button) {
        supported_caps << " middle_button";
        m_has_middle_button = true;
    }
    if (caps.palm_detect)
        supported_caps << " palm_detect";
    if (caps.multi_finger)
        supported_caps << " multi_finger";
    if (caps.ballistics)
        supported_caps << " ballistics";
    if (caps.four_buttons)
        supported_caps << " four_buttons";
    if (caps.sleep)
        supported_caps << " sleep";
    if (caps.multi_finger_report)
        supported_caps << " multi_finger_report";
    if (caps.low_power)
        supported_caps << " low_power";
    if (caps.pass_through)
        supported_caps << " pass_through";

    if (max_page >= 9) {
        auto extended = query<ExtendedModelIdPage>(9);

        if (extended.extended_w_mode) {
            supported_caps << " ew_mode";
            m.disgest_or_ewmode = 1;
        }
        if (extended.horizontal_scroll)
            supported_caps << "horiz_scroll";
        if (extended.vertical_scroll)
            supported_caps << "vert_scroll";
        if (extended.vertical_wheel)
            supported_caps << "vert_wheel";

        if (extended.number_of_extended_buttons)
            supported_caps << " " << extended.number_of_extended_buttons << "buttons";
    }

    if (max_page >= 0xC) {
        auto extended = query<ExtendedModelIdPage2>(0xC);

        if (extended.reports_v)
            supported_caps << " V";
        if (extended.clk_pad)
            m_middle_button_is_primary = true;
    }

    SYN_DEBUG << "supported caps ->" << supported_caps;
    set_mode(m);

    // Set the secret undocumented "advanced gesture" mode that makes the touchpad report multiple fingers
    if (caps.multi_finger_report) {
        auto byte = encode_byte(0x03);
        for (auto bit : byte.bits) {
            send_command(0xE8);
            send_command(bit);
        }
    }
    send_command(0xF3);
    send_command(0xC8);

    enable_irq();
}

SynapticsTouchpad::EncodedByte SynapticsTouchpad::encode_byte(u8 byte)
{
    EncodedByte enc {};

    for (size_t i = 4; i-- > 0;)
        enc.bits[3 - i] = (byte >> (i * 2)) & 0b11;

    return enc;
}

void SynapticsTouchpad::set_mode(Mode mode)
{
    auto encoded = encode_byte(bit_cast<u8>(mode));

    for (auto bits : encoded.bits) {
        send_command(0xE8);
        send_command(bits);
    }

    // "If a Set Sample Rate 20 ($F3, $14) command is preceded by four Set Resolution commands encoding
    //  an 8-bit argument, the 8-bit argument is stored as the new value for the TouchPad mode byte"
    send_command(0xF3);
    send_command(0x14);
}

template <typename T>
T SynapticsTouchpad::query(u8 page)
{
    u8 packet[3] {};

    auto encoded_page = encode_byte(page);

    for (auto bits : encoded_page.bits) {
        send_command(0xE8);
        send_command(bits);
    }

    send_command(0xE9);

    for (auto& byte : packet)
        byte = read_data();

    return bit_cast<T>(packet);
}

bool SynapticsTouchpad::handle_action()
{
    if (!data_available())
        return false;

    m_packet_buffer[m_offset_within_packet++] = read_data();
    if (m_offset_within_packet != 6)
        return true;

    m_offset_within_packet = 0;

    auto abs_packet = bit_cast<AbsolutePacket>(m_packet_buffer);

    u8 w = 4;

    if (m_supports_w) {
        w = abs_packet.w_0;
        w |= abs_packet.w_1 << 1;
        w |= abs_packet.w_2_to_3 << 2;
    }

    // Skip all extended W packets for now
    if (w == 2)
        return true;

    u16 x = abs_packet.x;
    x |= abs_packet.x_8_to_11 << 8;
    x |= abs_packet.x_12 << 12;

    u16 y = abs_packet.y;
    y |= abs_packet.y_8_to_11 << 8;
    y |= abs_packet.y_12 << 12;

    u16 z = abs_packet.z_pressure;

    i16 scroll_delta = 0;
    i16 x_delta = 0;
    i16 y_delta = 0;

    bool left_click = false;

    // Width too big or pressure too low, invalid coordinates, consider this a release event
    if (z < 20 || w >= 6 || x < 100 || y < 100) {
        // previous was release too
        if (m_prev_state.touch_begin_ts == 0)
            return true;

        auto press_diff = Timer::nanoseconds_since_boot() - m_prev_state.touch_begin_ts;
        if (press_diff <= (Time::nanoseconds_in_millisecond * 100))
            left_click = true;

        m_prev_state.touch_begin_ts = 0;
    } else {
        // previous state was release, record ts
        if (m_prev_state.touch_begin_ts == 0) {
            m_prev_state.touch_begin_ts = Timer::nanoseconds_since_boot();
        } else {
            i16 x_units_delta = static_cast<i16>(m_prev_state.x) - static_cast<i16>(x);
            x_units_delta *= -1;

            i16 y_units_delta = static_cast<i16>(m_prev_state.y) - static_cast<i16>(y);
            y_units_delta *= -1;

            if (w == 0) {
                // this is a scroll
                if (m_prev_state.finger_count == 2) {
                    scroll_delta = y_units_delta / (m_delta_per_pixel_y * 4);
                    scroll_delta *= -1;
                }

                m_prev_state.finger_count = 2;
            } else if (w == 1) {
                m_prev_state.finger_count = 3;
            } else {
                // Ignore delta if we had multiple fingers on the touchpad previously
                if (m_prev_state.finger_count == 0) {
                    y_delta = y_units_delta / m_delta_per_pixel_y;
                    x_delta = x_units_delta / m_delta_per_pixel_x;
                }

                m_prev_state.finger_count = 0;
            }
        }

        m_prev_state.x = x;
        m_prev_state.y = y;
    }

    Packet parsed_packet {};
    parsed_packet.left_button_state = (abs_packet.left || left_click) ? VKState::PRESSED : VKState::RELEASED;
    parsed_packet.right_button_state = abs_packet.right ? VKState::PRESSED : VKState::RELEASED;

    auto middle_btn_state = (abs_packet.l_u ^ abs_packet.left) ? VKState::PRESSED : VKState::RELEASED;

    if (m_middle_button_is_primary)
        parsed_packet.left_button_state = middle_btn_state;
    else if (m_has_middle_button)
        parsed_packet.middle_button_state = middle_btn_state;

    if (scroll_delta) {
        parsed_packet.scroll_direction = ScrollDirection::VERTICAL;
        parsed_packet.wheel_delta = scroll_delta;
    }
    if (x_delta)
        parsed_packet.x_delta = x_delta;
    if (y_delta)
        parsed_packet.y_delta = y_delta;

    EventManager::the().post_action(parsed_packet);
    return true;
}

}
