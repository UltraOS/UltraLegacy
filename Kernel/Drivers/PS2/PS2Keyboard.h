#pragma once

#include "Drivers/Keyboard.h"
#include "PS2Device.h"

namespace kernel {

class PS2Keyboard : public PS2Device, public Keyboard {
public:
    PS2Keyboard(PS2Controller::Channel channel);

    void handle_action() override;

    StringView name() const override { return "PS2 Keyboard"; }
};
}
