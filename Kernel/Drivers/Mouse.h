#pragma once

#include "Drivers/Device.h"
#include "WindowManager/Event.h"
#include "WindowManager/VirtualKey.h"

namespace kernel {

class Mouse : public Device {
public:
    Type device_type() const override { return Type::MOUSE; }

    struct Packet {
        ScrollDirection scroll_direction;

        i8 wheel_delta;

        i16 x_delta;
        i16 y_delta;

        VKState left_button_state;
        VKState middle_button_state;
        VKState right_button_state;
        VKState button_4_state;
        VKState button_5_state;
    };
};
}
