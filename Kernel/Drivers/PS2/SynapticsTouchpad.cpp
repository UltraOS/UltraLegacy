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
        set_units_per_pixel(res.x_units_per_mm / 8, res.y_units_per_mm / 8);
    } else {
        SYN_WARN << "resolution query returned bogus data, setting defaults";
        set_units_per_pixel(40 / 8, 64 / 8);
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
        if (extended.clk_pad) {
            declare_single_button_touchpad();
            m_has_middle_button = true;
        }
    }

    SYN_DEBUG << "supported caps ->" << supported_caps;
    set_mode(m);

    // Set the secret undocumented "advanced gesture" mode that makes the touchpad report multiple fingers
    if (caps.multi_finger_report) {
        m_supports_multifinger = true;

        auto byte = encode_byte(0x03);
        for (auto bit : byte.bits) {
            send_command(0xE8);
            send_command(bit);
        }
    }
    send_command(0xF3);
    send_command(0xC8);

    declare_primary_finger_memory();

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

    static constexpr u8 two_fingers = 0;
    static constexpr u8 three_or_more_fingers = 1;
    static constexpr u8 extended_packet = 2;
    static constexpr u8 pass_through = 3;

    // Don't care
    if (w == pass_through)
        return true;

    Update update {};

    if (w == extended_packet) {
        auto packet_code = (m_packet_buffer[5] >> 4) & 0xF;

        static constexpr u8 secondary_finger_packet = 1;
        static constexpr u8 finger_state_packet = 2;

        if (packet_code == secondary_finger_packet) {
            if (m_has_sent_second_finger) // must be a third/N finger packet. ignore it.
                return true;

            auto finger_packet = bit_cast<SecondaryFingerPacket>(m_packet_buffer);

            update.x = finger_packet.x_1_to_8 << 1;
            update.x |= finger_packet.x_9_to_12 << 9;

            update.y = finger_packet.y_1_to_8 << 1;
            update.y |= finger_packet.y_9_to_12 << 9;

            if (update.x < 100 || update.y < 100)
                update.x = update.y = 0;

            update.finger_count = m_finger_count;
            update.finger_index = 1;

            m_has_sent_second_finger = true;
            submit_update(update);
            return true;
        }

        if (packet_code == finger_state_packet) {
            auto reported_count = m_packet_buffer[1] & 0xF;

            if (m_finger_count >= 3 && reported_count >= 3) // otherwise, out of sync packet
                m_finger_count = reported_count;

            return true;
        }

        return true;
    }

    update.left_button = abs_packet.left ? VKState::PRESSED : VKState::RELEASED;
    update.right_button = abs_packet.right ? VKState::PRESSED : VKState::RELEASED;

    if (m_has_middle_button)
        update.middle_button = (abs_packet.l_u ^ abs_packet.left) ? VKState::PRESSED : VKState::RELEASED;

    update.x = abs_packet.x;
    update.x |= abs_packet.x_8_to_11 << 8;
    update.x |= abs_packet.x_12 << 12;

    update.y = abs_packet.y;
    update.y |= abs_packet.y_8_to_11 << 8;
    update.y |= abs_packet.y_12 << 12;

    u16 z = abs_packet.z_pressure;

    if (m_supports_multifinger) {
        if (w == two_fingers)
            m_finger_count = 2;
        else if (w == three_or_more_fingers && m_finger_count < 3) // otherwise, count is updated via finger state packet
            m_finger_count = 3;
        else if (w >= 4)
            m_finger_count = 1;
    } else {
        m_finger_count = 1;
    }

    // invalid values == empty packet
    if (z < 20 || w >= 6 || update.x < 100 || update.y < 100) {
        update.x = update.y = 0;
        m_finger_count = 0;
    }

    update.finger_count = m_finger_count;

    submit_update(update);
    m_has_sent_second_finger = false;

    return true;
}

}
