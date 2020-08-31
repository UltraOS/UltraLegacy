#pragma once

#include "Drivers/Device.h"

namespace kernel {

class Keyboard : public Device {
public:
    Type type() const override { return Type::KEYBOARD; }

    static void discover_and_setup();

    static Keyboard& the()
    {
        ASSERT(s_device != nullptr);

        return *s_device;
    }

private:
    static Keyboard* s_device;
};
}
