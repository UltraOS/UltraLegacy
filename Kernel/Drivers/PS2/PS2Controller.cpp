#include "PS2Controller.h"
#include "PS2Keyboard.h"

namespace kernel {

PS2Controller* PS2Controller::s_instance;

void PS2Controller::initialize()
{
    s_instance = new PS2Controller();
    s_instance->discover_all_devices();
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
    auto config                       = read_configuration();
    config.port_1_irq_enabled         = false;
    config.port_2_irq_enabled         = false;
    config.port_1_translation_enabled = false;

    // potentially
    bool is_dual_channel = config.port_2_clock_disabled;

    write_configuration(config);

    static constexpr u8 controller_test_passed = 0x55;

    // perform controller tests
    send_command(Command::TEST_CONTROLLER);
    auto controller_test_result = read_data();

    if (controller_test_result != controller_test_passed) {
        error() << "PS2Controller: self test failed, expected: 0x55 received: " << format::as_hex
                << controller_test_result;
        hang();
    }

    // restore the configuration again because it might've reset
    write_configuration(config);

    // check if it's actually a dual channel
    if (is_dual_channel) {
        send_command(Command::ENABLE_PORT_2);
        auto config     = read_configuration();
        is_dual_channel = !config.port_2_clock_disabled;

        if (is_dual_channel)
            send_command(Command::DISABLE_PORT_2);
    }

    // perform channel tests
    static constexpr u8 port_test_passed = 0x00;

    send_command(Command::TEST_PORT_1);
    auto port1_test_result = read_data();
    bool port1_passed      = port1_test_result == port_test_passed;

    u8   port2_test_result = 0xFF;
    bool port2_passed      = false;

    if (is_dual_channel) {
        send_command(Command::TEST_PORT_2);
        port2_test_result = read_data();
        port2_passed      = port2_test_result == port_test_passed;
    }

    // Initialize all available devices
    if (port1_passed) {
        send_command(Command::ENABLE_PORT_1);

        if (!reset_device(Channel::ONE)) {
            log() << "PS2Controller: Port 1 doesn't have a device attached";
        }

        discover_device(Channel::ONE);
    } else
        warning() << "PS2Controller: port 1 failed self test, expected: 0x00 received: " << format::as_hex
                  << port1_test_result;

    if (port2_passed) {
        send_command(Command::ENABLE_PORT_1);

        if (!reset_device(Channel::TWO)) {
            log() << "PS2Controller: Port 2 doesn't have a device attached";
        }

        discover_device(Channel::TWO);
    } else if (is_dual_channel && !port2_passed)
        warning() << "PS2Controller: port 2 failed self test, expected: 0x00 received: " << format::as_hex
                  << port2_test_result;

    if (!port1_passed && !port2_passed) {
        error() << "PS2Controller: All ports failed self test!";
        hang();
    }
}

bool PS2Controller::reset_device(Channel channel)
{
    send_command_to_device(channel, DeviceCommand::RESET, false);

    u8 device_response[3];
    device_response[0] = read_data(true);
    device_response[1] = read_data(true);
    device_response[2] = read_data(true);

    if ((device_response[0] == command_ack && device_response[1] == self_test_passed)
        || (device_response[0] == self_test_passed && device_response[1] == command_ack)) {
        return true;
    } else {
        if (!did_last_read_timeout()) {
            error() << "PS2Controller: Device " << static_cast<u8>(channel)
                    << " is present but responded with unknown sequence -> " << format::as_hex << device_response[0]
                    << "-" << device_response[1];
            hang();
        }

        log() << "PS2Controller: Device " << static_cast<u8>(channel) << " is not present.";
        return false;
    }
}

void PS2Controller::discover_device(Channel channel)
{
    // identify device
    send_command_to_device(channel, DeviceCommand::DISABLE_SCANNING);
    send_command_to_device(channel, DeviceCommand::IDENTIFY);

    u8 response_byte_count = 0;

    read_data(true);
    response_byte_count += !!did_last_read_timeout();

    read_data(true);
    response_byte_count += !!did_last_read_timeout();

    switch (response_byte_count) {
    case 0:
    case 2:
        log() << "Detected a keyboard on channel " << static_cast<u8>(channel);
        initialzie_keyboard(channel);
        return;
    case 1:
        log() << "Detected a mouse on channel " << static_cast<u8>(channel);
        initialize_mouse(channel);
        return;
    }
}

void PS2Controller::initialize_mouse(Channel channel)
{
    // detect the actual mouse type and initialize
    (void)channel;
}

void PS2Controller::initialzie_keyboard(Channel channel)
{
    // detect the actual keyboard type and initialize
    m_devices.emplace(new PS2Keyboard(channel));
}

bool PS2Controller::should_resend()
{
    auto data = read_data();

    if (data == command_ack)
        return false;

    if (data == resend_command)
        return true;

    error() << "PS2Controller: unexpected data instead of ACK/resend -> " << format::as_hex << data;
    hang();
}

void PS2Controller::send_command_to_device(Channel channel, DeviceCommand command, bool should_expect_ack)
{
    send_command_to_device(channel, static_cast<u8>(command), should_expect_ack);
}

void PS2Controller::send_command_to_device(Channel channel, u8 command, bool should_expect_ack)
{
    size_t resend_counter = 1000;

    do {
        switch (channel) {
        case Channel::TWO: send_command(Command::WRITE_PORT_2); [[fallthrough]];
        case Channel::ONE: write<data_port>(command); break;
        default: error() << "PS2Controller: Unknown channel " << static_cast<u8>(channel); hang();
        }
    } while (should_expect_ack && --resend_counter && should_resend());

    if (should_expect_ack && !resend_counter) {
        error() << "PS2Controller: failed to receive command ACK after 1000 attempts";
        hang();
    }
}

PS2Controller::Status PS2Controller::status()
{
    auto raw_status = IO::in8<status_port>();

    return *reinterpret_cast<Status*>(&raw_status);
}

PS2Controller::Configuration PS2Controller::read_configuration()
{
    send_command(Command::READ_CONFIGURATION);

    auto config = read_data();

    return *reinterpret_cast<Configuration*>(&config);
}

void PS2Controller::write_configuration(Configuration config)
{
    send_command(Command::WRITE_CONFIGURATION);
    write<data_port>(*reinterpret_cast<u8*>(&config));
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
