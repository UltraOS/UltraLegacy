#pragma once

#include "Drivers/Device.h"
#include "Interrupts/IRQHandler.h"
#include "PS2Controller.h"

namespace kernel {

class PS2Device : public IRQHandler {
public:
    static constexpr u8 port_1_irq_number = 1;
    static constexpr u8 port_2_irq_number = 12;

    explicit PS2Device(PS2Controller* parent, PS2Controller::Channel channel)
        : IRQHandler(IRQHandler::Type::LEGACY, channel == PS2Controller::Channel::ONE ? port_1_irq_number : port_2_irq_number)
        , m_controller(parent)
        , m_channel(channel)
    {
    }

    PS2Controller& controller() { return *m_controller; }

    [[nodiscard]] PS2Controller::Channel channel() const { return m_channel; }

protected:
    virtual bool handle_action() = 0;

    template <typename CommandT>
    void send_command(CommandT command)
    {
        m_controller->send_command_to_device(m_channel, static_cast<u8>(command));
    }

    bool data_available()
    {
        auto status = m_controller->status();

        auto channel = status.data_from_port_2 ? PS2Controller::Channel::TWO : PS2Controller::Channel::ONE;

        return status.output_full && (channel == m_channel);
    }

    u8 read_data() { return m_controller->read_data(); }

private:
    bool handle_irq(RegisterState&) override { return handle_action(); }

private:
    PS2Controller* m_controller;
    PS2Controller::Channel m_channel;
};
}
