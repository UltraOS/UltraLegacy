#include "PS2Controller.h"
#include "Core/RepeatUntil.h"
#include "PS2Keyboard.h"
#include "PS2Mouse.h"
#include "SynapticsTouchpad.h"

#define PS2_LOG log("PS2Controller")
#define PS2_WARN warning("PS2Controller")

#define PS2_DEBUG_MODE

#ifdef PS2_DEBUG_MODE
#define PS2_DEBUG log("PS2Controller")
#else
#define PS2_DEBUG DummyLogger()
#endif

namespace kernel {

PS2Controller::PS2Controller()
    : Device(Category::CONTROLLER)
{
}

void PS2Controller::discover_all_devices()
{
    // TODO: disable USB Legacy Support
    // TODO: check FADT to make sure PS2 controller exists

    // disable ports
    send_command(Command::DISABLE_PORT_1);
    send_command(Command::DISABLE_PORT_2);

    flush();

    // disable both irqs and translation
    auto config = read_configuration();
    config.port_1_irq_enabled = false;
    config.port_2_irq_enabled = false;
    config.port_1_translation_enabled = false;

    // potentially
    bool is_dual_channel = config.port_2_clock_disabled;

    write_configuration(config);

    static constexpr u8 controller_test_passed = 0x55;

    // perform controller tests
    send_command(Command::TEST_CONTROLLER);
    auto controller_test_result = read_data();

    if (controller_test_result != controller_test_passed) {
        String error_string;
        error_string << "PS2Controller: self test failed, expected: 0x55 received: " << controller_test_result;
        runtime::panic(error_string.data());
    }

    // restore the configuration again because it might've reset
    write_configuration(config);

    // check if it's actually a dual channel
    if (is_dual_channel) {
        send_command(Command::ENABLE_PORT_2);
        auto config = read_configuration();
        is_dual_channel = !config.port_2_clock_disabled;

        if (is_dual_channel)
            send_command(Command::DISABLE_PORT_2);
    }

    // perform channel tests
    bool port1_passed = test_port(Channel::ONE);
    bool port2_passed = false;

    if (is_dual_channel)
        port2_passed = test_port(Channel::TWO);

    bool device_1_present = false;
    bool device_2_present = false;

    // Attempt to detect multiplexing capability
    if (port1_passed && port2_passed) {
        write<command_port>(0xD3);
        write<data_port>(0xF0);
        read_data();

        write<command_port>(0xD3);
        write<data_port>(0x56);
        read_data();

        write<command_port>(0xD3);
        write<data_port>(0xA4);
        u8 version_byte = read_data();

        if (version_byte == 0xA4) {
            PS2_LOG << "no multiplexing capabilities detected";
        } else {
            m_multiplexed_mode = true;

            u8 major = (version_byte & 0xF0) >> 4;
            u8 minor = version_byte & 0x0F;

            PS2_LOG << "multiplexing mode enabled, version " << major << "." << minor;
        }
    }

    // Make sure all ports are disabled to avoid race conditions
    if (is_multiplexed()) {
        auto channel = Channel::AUX_ZERO;
        while (channel <= Channel::AUX_THREE) {
            disable(channel);
            channel += 1;
        }
        flush();
    }

    // Initialize all available devices
    if (port1_passed)
        device_1_present |= initialize_device_if_present(Channel::ONE);

    bool aux_present[4] {};

    if (port2_passed) {
        if (m_multiplexed_mode) {
            auto channel = Channel::AUX_ZERO;
            size_t aux_offset = 0;
            while (channel <= Channel::AUX_THREE) {
                bool did_initialize = initialize_device_if_present(channel);
                device_2_present |= did_initialize;
                aux_present[aux_offset++] = did_initialize;
                channel += 1;
            }
        } else {
            device_2_present |= initialize_device_if_present(Channel::TWO);
        }
    }

    if (!port1_passed && !port2_passed)
        runtime::panic("PS2Controller: All ports failed self test!");

    if (device_1_present) {
        enable(Channel::ONE);
        send_command_to_device(Channel::ONE, DeviceCommand::ENABLE_SCANNING);
    }

    if (is_multiplexed()) {
        auto channel = Channel::AUX_ZERO;

        for (size_t i = 0; i < 4; ++i, channel += 1) {
            if (!aux_present[i])
                continue;

            enable(channel);
            send_command_to_device(channel, DeviceCommand::ENABLE_SCANNING);
        }
    } else if (device_2_present) {
        enable(Channel::TWO);
        send_command_to_device(Channel::TWO, DeviceCommand::ENABLE_SCANNING);
    }

    // TODO: if for whatever reason keyboard ends up on port 2
    // we have to set scancode set 1 for it manually, or just support set 2
    // because we cannot have auto translation for port 2
    // TODO: disable translation if keyboard ends up on port 2
    config.port_1_irq_enabled = device_1_present;
    config.port_1_translation_enabled = device_1_present;
    config.port_1_clock_disabled = !device_1_present;

    config.port_2_irq_enabled = device_2_present;
    config.port_2_clock_disabled = !device_2_present;

    write_configuration(config);
}

bool PS2Controller::test_port(Channel on_channel)
{
    static constexpr u8 port_test_passed = 0x00;

    auto test_command = on_channel == Channel::ONE ? Command::TEST_PORT_1 : Command::TEST_PORT_2;

    send_command(test_command);
    u8 result = read_data();

    if (result != port_test_passed)
        PS2_WARN << "port " << to_string(on_channel) << " failed self test, expected: 0x00 received: " << format::as_hex << result;

    return result == port_test_passed;
}

void PS2Controller::enable(Channel channel)
{
    auto enable_port = channel == Channel::ONE ? Command::ENABLE_PORT_1 : Command::ENABLE_PORT_2;

    if (channel != Channel::ONE && is_multiplexed())
        send_command(static_cast<Command>(channel));

    send_command(enable_port);
    flush();
}

void PS2Controller::disable(Channel channel)
{
    // Disable the port until we initialize all devices
    auto disable_port = channel == Channel::ONE ? Command::DISABLE_PORT_1 : Command::DISABLE_PORT_2;

    if (channel != Channel::ONE && is_multiplexed())
        send_command(static_cast<Command>(channel));

    send_command(disable_port);
    flush();
}

bool PS2Controller::initialize_device_if_present(Channel channel)
{
    enable(channel);

    // Disable scanning so that we avoid any race conditions with user spamming buttons
    // while we're trying to initialize the device. If there's no device on this port
    // it will simply timeout.
    send_command_to_device(channel, DeviceCommand::DISABLE_SCANNING, false);
    for (;;) {
        repeat_until(
            []() -> bool {
                return status().output_full;
            },
            30);
        auto data = read_data(true, 100);
        if (did_last_read_timeout() || data == resend_command)
            break;
        if (data != command_ack)
            PS2_WARN << "got garbage instead of ACK -> " << format::as_hex << data << ", ignored";
    }
    flush();

    if (!reset_device(channel)) {
        disable(channel);
        return false;
    }

    send_command_to_device(channel, DeviceCommand::DISABLE_SCANNING);
    auto device = identify_device(channel);

    if (device.is_keyboard()) {
        PS2_LOG << "Detected a keyboard on channel " << to_string(channel);
        (new PS2Keyboard(this, channel))->make_child_of(this);
    } else if (device.is_mouse()) {
        // try to detect synaptics touchpad
        for (size_t i = 0; i < 4; ++i) {
            send_command_to_device(channel, 0xE8);
            send_command_to_device(channel, 0x00);
        }

        send_command_to_device(channel, 0xE9);

        u8 packet[3] {};
        for (auto& byte : packet)
            byte = read_data();

        if (packet[1] == SynapticsTouchpad::magic_constant) {
            PS2_LOG << "Detected a Synaptics TouchPad on channel " << to_string(channel);
            (new SynapticsTouchpad(this, channel, bit_cast<SynapticsTouchpad::IdentificationPage>(packet)))->make_child_of(this);
        } else {
            PS2_LOG << "Detected a mouse on channel " << to_string(channel);
            (new PS2Mouse(this, channel))->make_child_of(this);
        }
    }

    disable(channel);

    return true;
}

bool PS2Controller::reset_device(Channel channel)
{
    send_command_to_device(channel, DeviceCommand::RESET, false);

    WaitResult wait_res {};

    for (;;) {
        wait_res = repeat_until(
            []() -> bool {
                return status().output_full;
            },
            30);

        if (!wait_res.success) {
            PS2_LOG << "no device on channel " << to_string(channel);
            return false;
        }

        auto ack_or_resend = read_data(true, 2500);
        if (ack_or_resend == resend_command) {
            PS2_LOG << "reset returned 0xFE for channel " << to_string(channel)
                    << ", assuming timeout reached and no device present";
            return false;
        }

        if (ack_or_resend == command_ack)
            break;

        PS2_WARN << "garbage byte instead of ACK - " << format::as_hex << ack_or_resend << ", ignored";
    }

    // Allow about 2 seconds for the reset to complete.
    // From Synaptics touchpads guide:
    // "Power-on self-test and calibration normally takes 300–1000 ms. Self-test and calibration following a
    //  software Reset command normally takes 300–500 ms. In most Synaptics TouchPads, the delays are
    //  nominally 750 ms and 350 ms, respectively."
    // According to what I've seen this usually takes about half a second.
    for (;;) {
        wait_res = repeat_until(
            []() -> bool {
                return status().output_full;
            },
            2000);

        if (!wait_res.success) {
            PS2_WARN << "reset timeout out for device on channel " << to_string(channel);
            return false;
        }

        u8 data = read_data(false, 100);
        if (data == reset_failure) {
            PS2_WARN << "device on channel " << to_string(channel) << " failed to reset";
            return false;
        }

        if (data == self_test_passed) {
            PS2_DEBUG << "reset device on channel " << to_string(channel) << " in " << wait_res.elapsed_ms << "MS";

            // flush all garbage bytes we might get
            for (;;) {
                repeat_until(
                    []() -> bool {
                        return status().output_full;
                    },
                    30);
                read_data(true, 100);

                if (did_last_read_timeout())
                    break;
            }

            return true;
        }

        PS2_WARN << "got a garbage byte " << format::as_hex << data << " after device reset, ignored";
    }
}

PS2Controller::DeviceIdentification PS2Controller::identify_device(Channel channel)
{
    send_command_to_device(channel, DeviceCommand::IDENTIFY);

    DeviceIdentification ident {};

    for (;;) {
        repeat_until(
            []() -> bool {
                return status().output_full;
            },
            30);

        auto id_byte = read_data(true, 100);

        if (did_last_read_timeout())
            break;

        if (ident.id_bytes == 0) {
            if (id_byte != 0x00 && id_byte != 0x03 && id_byte != 0x04 && id_byte != 0xAB) {
                PS2_WARN << "unexpected first id byte " << format::as_hex << id_byte << ", skipped";
                continue;
            }

            ident.id[0] = id_byte;
        } else {
            if (id_byte != 0x41 && id_byte != 0xC1 && id_byte != 0x83) {
                PS2_WARN << "unexpected second id byte " << format::as_hex << id_byte
                         << ", first was " << ident.id[0] << ", skipped";
                continue;
            }

            ident.id[1] = id_byte;
        }

        if (ident.id_bytes++ == 1)
            break;
    }

    return ident;
}

bool PS2Controller::should_resend()
{
    for (;;) {
        auto data = read_data();

        if (data == command_ack)
            return false;

        if (data == resend_command)
            return true;

        PS2_WARN << "unexpected byte instead of ACK/resend -> " << format::as_hex << data << ", ignored";
    }
}

void PS2Controller::send_command_to_device(Channel channel, DeviceCommand command, bool should_expect_ack)
{
    send_command_to_device(channel, static_cast<u8>(command), should_expect_ack);
}

void PS2Controller::send_command_to_device(Channel channel, u8 command, bool should_expect_ack)
{
    size_t resend_counter = 3;

    do {
        if (channel != Channel::ONE)
            send_command(static_cast<Command>(channel));

        write<data_port>(command);
    } while (should_expect_ack && --resend_counter && should_resend());

    if (should_expect_ack && !resend_counter) {
        runtime::panic("PS2Controller: failed to receive command ACK after 3 attempts");
    }
}

u8 PS2Controller::read_data(bool allow_failure, size_t max_attempts)
{
    while (!status().output_full && --max_attempts)
        pause();

    if (max_attempts == 0) {
        if (allow_failure) {
            m_last_read_timeout = true;
            return 0x00;
        }

        runtime::panic("PS2Controller: unexpected read timeout!");
    }

    if (allow_failure)
        m_last_read_timeout = false;

    return IO::in8<data_port>();
}

PS2Controller::Status PS2Controller::status()
{
    auto raw_status = IO::in8<status_port>();

    return bit_cast<Status>(raw_status);
}

PS2Controller::Configuration PS2Controller::read_configuration()
{
    send_command(Command::READ_CONFIGURATION);

    auto raw_config = read_data();
    return bit_cast<Configuration>(raw_config);
}

void PS2Controller::write_configuration(Configuration config)
{
    send_command(Command::WRITE_CONFIGURATION);

    auto raw_config = bit_cast<u8>(config);
    write<data_port>(raw_config);
}

void PS2Controller::send_command(Command command)
{
    write<command_port>(static_cast<u8>(command));
}

void PS2Controller::flush()
{
    while (status().output_full)
        IO::in8<data_port>();
}
}
