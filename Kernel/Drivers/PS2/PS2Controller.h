#pragma once

#include "Drivers/Device.h"
#include "Common/DynamicArray.h"

namespace kernel {

class PS2Controller : public Device {
public:
    Type type() const override { return Type::CONTROLLER; }
    StringView name() const override { return "8042 PS/2 Controller"; }

    PS2Controller();

    static void initialize();

    static PS2Controller& the()
    {
        ASSERT(s_instance != nullptr);

        return *s_instance;
    }

private:
    DynamicArray<Device*> m_devices;

    static PS2Controller* s_instance;
};
}
