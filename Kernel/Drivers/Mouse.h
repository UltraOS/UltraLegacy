#pragma once

#include "Drivers/Device.h"

namespace kernel {

class Mouse : public Device {
public:
    Type type() const override { return Type::MOUSE; }
};
}
