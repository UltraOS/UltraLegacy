#pragma once

#include "Common/DynamicArray.h"
#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Core/Runtime.h"
#include "PCI.h"
namespace kernel {

class Access {
public:
    static Access* detect();

    virtual u32 read32(PCI::Location, u32 offset) = 0;
    virtual void write32(PCI::Location, u32 offset, u32 value) = 0;

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

    void write16(PCI::Location location, u32 offset, u16 value)
    {
        auto res = read32(location, offset & 0xFC);

        res |= value << (offset & 2);

        write32(location, offset & 0xFC, res);
    }

    void write8(PCI::Location location, u32 offset, u8 value)
    {
        auto res = read32(location, offset & 0xFC);

        res |= value << (offset & 3);

        write32(location, offset & 0xFC, res);
    }

    virtual DynamicArray<PCI::DeviceInfo> list_all_devices() = 0;
};

}
