#pragma once

#include "Common/DynamicArray.h"
#include "Common/Logger.h"

#include "Core/IO.h"

#include "Drivers/Device.h"

namespace kernel {

class PS2Device;

class PS2Controller : public Device {
public:
    static constexpr u8 data_port    = 0x60;
    static constexpr u8 status_port  = 0x64;
    static constexpr u8 command_port = 0x64;

    enum class Command : u8 {
        READ_CONFIGURATION  = 0x20,
        WRITE_CONFIGURATION = 0x60,
        DISABLE_PORT_2      = 0xA7,
        ENABLE_PORT_2       = 0xA8,
        TEST_PORT_2         = 0xA9,
        TEST_CONTROLLER     = 0xAA,
        TEST_PORT_1         = 0xAB,
        DISABLE_PORT_1      = 0xAD,
        ENABLE_PORT_1       = 0xAE,
        WRITE_PORT_2        = 0xD4,
    };

    static constexpr u8 self_test_passed = 0xAA;
    static constexpr u8 command_ack      = 0xFA;
    static constexpr u8 resend_command   = 0xFE;

    enum class DeviceCommand : u8 {
        RESET            = 0xFF,
        ENABLE_SCANNING  = 0xF4,
        DISABLE_SCANNING = 0xF5,
        IDENTIFY         = 0xF2,
    };

    struct Configuration {
        bool port_1_irq_enabled : 1;
        bool port_2_irq_enabled : 1;
        bool system_flag : 1;
        bool unused_1 : 1;
        bool port_1_clock_disabled : 1;
        bool port_2_clock_disabled : 1;
        bool port_1_translation_enabled : 1;
        bool unused_2 : 1;
    };

    struct Status {
        bool output_full : 1;
        bool input_full : 1;
        bool system_flag : 1;
        bool controller_data : 1;
        bool unknown_1 : 1;
        bool data_from_port_2 : 1;
        bool timeout_error : 1;
        bool parity_error : 1;
    };

    static_assert(sizeof(Configuration) == 1);
    static_assert(sizeof(Status) == 1);

    Type       type() const override { return Type::CONTROLLER; }
    StringView name() const override { return "8042 PS/2 Controller"; }

    static void initialize();

    static PS2Controller& the()
    {
        ASSERT(s_instance != nullptr);

        return *s_instance;
    }

    void send_command(Command);
    void flush();

    Configuration read_configuration();
    void          write_configuration(Configuration);

    Status status();

    enum class Channel { ONE = 1, TWO = 2 };

    void send_command_to_device(Channel, DeviceCommand, bool should_expect_ack = true);
    void send_command_to_device(Channel, u8, bool should_expect_ack = true);

    bool should_resend();

    template <u8 port>
    void write(u8 data)
    {
        size_t timeout_counter = 100000;

        while (status().input_full && --timeout_counter)
            pause();

        if (timeout_counter == 0) {
            runtime::panic("PS2Controller: unexpected write timeout!");
        }

        IO::out8<port>(data);
    }

    u8 read_data(bool allow_failure = false, size_t max_attempts = 100000);

    bool did_last_read_timeout() { return m_last_read_timeout; }

    struct DeviceIdentification {
        u8 id_bytes;
        u8 id[2];

        bool is_keyboard() { return id_bytes == 0 || id_bytes == 2; }
        bool is_mouse() { return id_bytes == 1; }
    };

    DeviceIdentification identify_device(Channel);

    bool test_port(Channel);

private:
    void discover_all_devices();

    bool reset_device(Channel);

    bool initialize_device_if_present(Channel);

private:
    DynamicArray<PS2Device*> m_devices;
    bool                     m_last_read_timeout;

    static PS2Controller* s_instance;
};
}
