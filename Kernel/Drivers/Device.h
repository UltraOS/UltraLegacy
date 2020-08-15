#pragma once
#include "Common/String.h"

namespace kernel {

class Device {
public:
    enum class Type {
        VIDEO,
        KEYBOARD,
        MOUSE,
        DISK,
        NETWORK,
        AUDIO,
    };

    virtual Type       type() const = 0;
    virtual StringView name() const = 0;

    virtual ~Device() = default;
};
}
