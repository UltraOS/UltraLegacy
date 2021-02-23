#pragma once

#include "Drivers/Device.h"
#include "WindowManager/VirtualKey.h"

namespace kernel {

class Keyboard : public Device {
public:
    Type device_type() const override { return Type::KEYBOARD; }

    struct Packet {
        VK key;
        VKState state;
    };
};
}
