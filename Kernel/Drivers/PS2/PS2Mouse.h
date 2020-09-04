#pragma once

#include "Drivers/Mouse.h"
#include "PS2Device.h"

namespace kernel {

class PS2Mouse : public PS2Device, public Mouse {
public:
    PS2Mouse(PS2Controller::Channel channel);

    void handle_action() override;

    StringView name() const override { return "PS2 Mouse"; }
};
}
