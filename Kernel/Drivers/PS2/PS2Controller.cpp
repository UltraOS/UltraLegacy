#include "PS2Controller.h"
#include "PS2Keyboard.h"
#include "PS2Mouse.h"

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
        StackStringBuilder error_string;
        error_string << "PS2Controller: self test failed, expected: 0x55 received: " << controller_test_result;
        runtime::panic(error_string.data());
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
    bool port1_passed = test_port(Channel::ONE);
    bool port2_passed = false;

    if (is_dual_channel)
        port2_passed = test_port(Channel::TWO);

    bool device_1_present = false;
    bool device_2_present = false;

    // Initialize all available devices
    if (port1_passed)
        device_1_present = initialize_device_if_present(Channel::ONE);

    if (port2_passed)
        device_2_present = initialize_device_if_present(Channel::TWO);

    if (!port1_passed && !port2_passed) {
        runtime::panic("PS2Controller: All ports failed self test!");
    }

    // This has to be done here and not in the device constructors
    // becase some firmware might send an IRQ for commands requesting data
    // and we don't want that to happen.
    if (device_1_present)
        send_command_to_device(Channel::ONE, DeviceCommand::ENABLE_SCANNING);
    if (device_2_present)
        send_command_to_device(Channel::TWO, DeviceCommand::ENABLE_SCANNING);

    // TODO: if for whatever reason keyboard ends up on port 2
    // we have to set scancode set 1 for it manually, or just support set 2
    // because we cannot have auto translation for port 2
    // TODO: disable translation if keyboard ends up on port 2
    config.port_1_irq_enabled         = device_1_present;
    config.port_1_translation_enabled = device_1_present;
    config.port_1_clock_disabled      = !device_1_present;

    config.port_2_irq_enabled    = device_2_present;
    config.port_2_clock_disabled = !device_2_present;

    write_configuration(config);
}

bool PS2Controller::test_port(Channel on_channel)
{
    static constexpr u8 port_test_passed = 0x00;

    auto test_command = on_channel == Channel::ONE ? Command::TEST_PORT_1 : Command::TEST_PORT_2;

    send_command(test_command);
    u8 result = read_data();

    if (result != port_test_passed) {
        warning() << "PS2Controller: port " << static_cast<u8>(on_channel)
                  << " failed self test, expected: 0x00 received: " << format::as_hex << result;
    }

    return result == port_test_passed;
}

bool PS2Controller::initialize_device_if_present(Channel channel)
{
    auto enable_port = channel == Channel::ONE ? Command::ENABLE_PORT_1 : Command::ENABLE_PORT_2;

    send_command(enable_port);

    if (!reset_device(channel)) {
        log() << "PS2Controller: Port " << static_cast<u8>(channel) << " doesn't have a device attached";
        return false;
    }

    auto device = identify_device(channel);

    if (device.is_keyboard()) {
        log() << "Detected a keyboard on channel " << static_cast<u8>(channel);
        m_devices.emplace(new PS2Keyboard(channel));
    } else if (device.is_mouse()) {
        log() << "Detected a mouse on channel " << static_cast<u8>(channel);
        m_devices.emplace(new PS2Mouse(channel));
    }

    return true;
}

bool PS2Controller::reset_device(Channel channel)
{
    // This command takes a ton of time on real hw (and apparanetly on VirtualBox too)
    // therefore we cannot afford a smaller delay in read_data()
    send_command_to_device(channel, DeviceCommand::RESET, false);

    u8 device_response[3];
    device_response[0] = read_data(true);
    device_response[1] = read_data(true);
    device_response[2] = read_data(true, 250); // unlikely to not timeout?

    if ((device_response[0] == command_ack && device_response[1] == self_test_passed)
        || (device_response[0] == self_test_passed && device_response[1] == command_ack)) {
        return true;
    } else {
        if (!did_last_read_timeout()) {
            StackStringBuilder error_string;
            error_string << "PS2Controller: Device " << static_cast<u8>(channel);
            error_string << " is present but responded with unknown sequence -> ";
            error_string.append_hex(device_response[0]);
            error_string += '-';
            error_string.append_hex(device_response[1]);
            error_string += '-';
            error_string.append_hex(device_response[2]);
            runtime::panic(error_string.data());
        }

        log() << "PS2Controller: Device " << static_cast<u8>(channel) << " is not present.";
        return false;
    }
}

PS2Controller::DeviceIdentification PS2Controller::identify_device(Channel channel)
{
    // identify device
    send_command_to_device(channel, DeviceCommand::DISABLE_SCANNING);
    send_command_to_device(channel, DeviceCommand::IDENTIFY);

    DeviceIdentification ident {};

    for (auto i = 0; i < 2; ++i) {
        ident.id[i] = read_data(true, 250); // this delay is super sensitive and prone to breaking
        ident.id_bytes += !!did_last_read_timeout();
    }

    return ident;
}

bool PS2Controller::should_resend()
{
    auto data = read_data();

    if (data == command_ack)
        return false;

    if (data == resend_command)
        return true;

    StackStringBuilder string;
    string += "PS2Controller: unexpected data instead of ACK/resend -> ";
    string.append_hex(data);
    runtime::panic(string.data());
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
        default: ASSERT_NEVER_REACHED();
        }
    } while (should_expect_ack && --resend_counter && should_resend());

    if (should_expect_ack && !resend_counter) {
        runtime::panic("PS2Controller: failed to receive command ACK after 1000 attempts");
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
