#pragma once

#include "Drivers/Device.h"
#include "Interrupts/IRQHandler.h"
#include "PS2Controller.h"

namespace kernel {

class PS2Device : public IRQHandler {
public:
    static constexpr u8 port_1_irq_number = 1;
    static constexpr u8 port_2_irq_number = 12;

    explicit PS2Device(PS2Controller::Channel channel)
        : IRQHandler(channel == PS2Controller::Channel::ONE ? port_1_irq_number : port_2_irq_number)
        , m_channel(channel)
    {
    }

    [[nodiscard]] PS2Controller::Channel channel() const { return m_channel; }

    template <typename CommandT>
    void send_command(CommandT command)
    {
        PS2Controller::the().send_command_to_device(m_channel, static_cast<u8>(command));
    }

    bool data_available()
    {
        auto status = PS2Controller::the().status();

        auto channel = status.data_from_port_2 ? PS2Controller::Channel::TWO : PS2Controller::Channel::ONE;

        return status.output_full && (channel == m_channel);
    }

    static u8 read_data() { return PS2Controller::the().read_data(); }

    void handle_irq(const RegisterState&) override { handle_action(); }

    virtual void handle_action() = 0;

private:
    PS2Controller::Channel m_channel;
};
}
