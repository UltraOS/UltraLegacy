#pragma once

#include "Core/Boot.h"

#include "Drivers/Device.h"

namespace kernel {

class VideoDevice : public Device {
public:
    Type type() const override { return Type::VIDEO; }

    virtual const VideoMode& mode() const = 0;

    static void discover_and_setup(const VideoMode&);

    static VideoDevice& the()
    {
        ASSERT(s_device != nullptr);

        return *s_device;
    }

private:
    static VideoDevice* s_device;
};
}
