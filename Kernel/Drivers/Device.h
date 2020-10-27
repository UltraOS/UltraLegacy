#pragma once
#include "Common/String.h"

namespace kernel {

class Device {
    MAKE_NONCOPYABLE(Device);

public:
    Device() = default;

    enum class Type {
        VIDEO,
        KEYBOARD,
        MOUSE,
        DISK,
        NETWORK,
        AUDIO,
        CONTROLLER,
    };

    virtual Type type() const = 0;
    virtual StringView name() const = 0;

    virtual ~Device() = default;
};
}
