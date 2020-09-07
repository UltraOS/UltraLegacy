#pragma once

#include "Drivers/Device.h"

namespace kernel {

class Mouse : public Device {
public:
    Type type() const override { return Type::MOUSE; }

    struct Packet {
        i8 wheel_delta;
        i8 x_delta;
        i8 y_delta;

        bool left_button_held;
        bool middle_button_held;
        bool right_button_held;
        bool button_4_held;
        bool button_5_held;
    };
};
}
