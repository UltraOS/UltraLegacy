#pragma once

#include "Drivers/Device.h"
#include "WindowManager/VirtualKey.h"

namespace kernel {

class Keyboard : public Device {
public:
    Keyboard()
        : Device(Category::KEYBOARD)
    {
    }

    struct Action {
        VK key;
        VKState state;
    };
};
}
