#pragma once

#include "Common/Macros.h"
#include "Core/Runtime.h"
#include "Common/DynamicArray.h"
#include "PCI.h"
#include "Common/Logger.h"
namespace kernel {

class Access {
public:
    static Access* detect();

    virtual u32 read32(PCI::Location, u32 offset) = 0;

    u16 read16(PCI::Location location, u32 offset)
    {
        auto res = read32(location, offset & 0xFC);
        res >>= (offset & 2) * 8;
        return res & 0xFFFF;
    }

    u8 read8(PCI::Location location, u32 offset)
    {
        auto res = read32(location, offset & 0xFC);
        res >>= (offset & 3) * 8;
        return res & 0xFF;
    }

    virtual DynamicArray<PCI::DeviceInfo> list_all_devices() = 0;
};

}
