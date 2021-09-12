#pragma once

#include "Drivers/TouchPad.h"
#include "PS2Controller.h"
#include "PS2Device.h"

namespace kernel {

class SynapticsTouchpad : public PS2Device, public TouchPad {
public:
    static constexpr u8 magic_constant = 0x47;

    struct PACKED IdentificationPage {
        u8 minor_version;
        u8 magic_constant;
        u8 major_version : 4;
        u8 model_code : 4;
    };
    struct PACKED CapabilitiesPage {
        u8 reserved : 2;
        u8 middle_button : 1;
        u8 reserved1 : 1;
        u8 max_query_page : 3;
        u8 has_extended_caps : 1;
        u8 model_sub_number_or_magic_constant;
        u8 palm_detect : 1;
        u8 multi_finger : 1;
        u8 ballistics : 1;
        u8 four_buttons : 1;
        u8 sleep : 1;
        u8 multi_finger_report : 1;
        u8 low_power : 1;
        u8 pass_through : 1;
    };
    struct PACKED ResolutionPage {
        u8 x_units_per_mm;
        u8 highest_bit_must_be_set;
        u8 y_units_per_mm;
    };
    struct PACKED ExtendedModelIdPage {
        u8 vertical_scroll : 1;
        u8 horizontal_scroll : 1;
        u8 extended_w_mode : 1;
        u8 vertical_wheel : 1;
        u8 glass_pass : 1;
        u8 peak_detect : 1;
        u8 light_control : 1;
        u8 reserved : 1;
        u8 reserved1 : 2;
        u8 info_sensor : 2;
        u8 number_of_extended_buttons : 4;
        u8 product_id;
    };
    struct PACKED ExtendedModelIdPage2 {
        u8 tb_adj_thresh : 1;
        u8 reports_max : 1;
        u8 clear_pad : 1;
        u8 advanced_gestures : 1;
        u8 clk_pad : 1;
        u8 multi_finger_mode : 2;
        u8 covered_pad_gest : 1;
        u8 clk_pad1 : 1;
        u8 deluxe_led : 1;
        u8 no_abs_pos_filt : 1;
        u8 reports_v : 1;
        u8 uniform_clickpad : 1;
        u8 reports_min : 1;
        u8 intertouch : 1;
        u8 reserved : 1;
        u8 intertouch_address;
    };
    static_assert(sizeof(IdentificationPage) == 3);
    static_assert(sizeof(CapabilitiesPage) == 3);
    static_assert(sizeof(ResolutionPage) == 3);
    static_assert(sizeof(ExtendedModelIdPage) == 3);
    static_assert(sizeof(ExtendedModelIdPage2) == 3);

    explicit SynapticsTouchpad(PS2Controller* parent, PS2Controller::Channel channel, const IdentificationPage&);

    [[nodiscard]] StringView device_type() const override { return "PS/2 Device"_sv; }
    [[nodiscard]] StringView device_model() const override { return "Synaptics PS/2 TouchPad"_sv; }

    struct PACKED Mode {
        u8 wmode : 1;
        u8 pack_size : 1;
        u8 disgest_or_ewmode : 1;
        u8 sleep : 1;
        u8 guest_acpi_mode : 1;
        u8 transparent_mode : 1;
        u8 rate : 1;
        u8 absolute : 1;
    };
    static_assert(sizeof(Mode) == 1);

    void set_mode(Mode);

private:
    struct EncodedByte {
        u8 bits[4];
    };

    static EncodedByte encode_byte(u8);

    template <typename T>
    T query(u8 page);

    bool handle_action() override;

    struct PACKED AbsolutePacket {
        u8 left : 1;
        u8 right : 1;
        u8 w_1 : 1;
        u8 always_zero : 1;
        u8 w_2_to_3 : 2;
        u8 always_zero1 : 1;
        u8 always_one : 1;
        u8 x_8_to_11 : 4;
        u8 y_8_to_11 : 4;
        u8 z_pressure;
        u8 l_u : 1;
        u8 r_d : 1;
        u8 w_0 : 1;
        u8 always_zero2 : 1;
        u8 x_12 : 1;
        u8 y_12 : 1;
        u8 always_one1 : 1;
        u8 always_one2 : 1;
        u8 x;
        u8 y;
    };

    struct PACKED SecondaryFingerPacket {
        u8 left : 1;
        u8 right : 1;
        u8 always_one : 1;
        u8 always_zero : 4;
        u8 always_one1 : 1;
        u8 x_1_to_8;
        u8 y_1_to_8;
        u8 left1 : 1;
        u8 right1 : 1;
        u8 always_zero2 : 2;
        u8 z_5_to_6 : 2;
        u8 always_one3 : 2;
        u8 x_9_to_12 : 4;
        u8 y_9_to_12 : 4;
        u8 z_1_to_4 : 4;
        u8 always_one4 : 4;
    };

    static_assert(sizeof(SecondaryFingerPacket) == 6);
    static_assert(sizeof(AbsolutePacket) == 6);

private:
    u8 m_packet_buffer[6] {};
    u8 m_offset_within_packet { 0 };

    bool m_has_middle_button { false };
    bool m_supports_w { false };

    bool m_supports_multifinger { false };

    // Used to control the order so the second finger doesn't get sent twice.
    bool m_has_sent_second_finger { false };

    u8 m_finger_count { 0 };
};

}
