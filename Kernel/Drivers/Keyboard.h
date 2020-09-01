#pragma once

#include "Drivers/Device.h"

namespace kernel {

class Keyboard : public Device {
public:
    Type type() const override { return Type::KEYBOARD; }
};
}
