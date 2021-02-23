#pragma once
#include "Common/String.h"

namespace kernel {

class Device {
    MAKE_NONCOPYABLE(Device);

public:
    Device() = default;

    enum class Type {
        TIMER,
        INTERRUPT_CONTROLLER,
        VIDEO,
        KEYBOARD,
        MOUSE,
        STORAGE,
        NETWORK,
        AUDIO,
        CONTROLLER,
    };

    virtual Type device_type() const = 0;
    virtual StringView device_name() const = 0;

    virtual ~Device() = default;
};
}
