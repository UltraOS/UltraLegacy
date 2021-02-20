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

    static constexpr u32 aligned_mask = 0b11111100;

    u16 read16(PCI::Location location, u32 offset)
    {
        auto res = read32(location, offset & aligned_mask);
        res >>= (offset & 2) * 8;
        return res & 0xFFFF;
    }

    u8 read8(PCI::Location location, u32 offset)
    {
        auto res = read32(location, offset & aligned_mask);
        res >>= (offset & 3) * 8;
        return res & 0xFF;
    }

    void write16(PCI::Location location, u32 offset, u16 value)
    {
        auto res = read32(location, offset & aligned_mask);

        bool upper = offset & 2;

        if (upper)
            res = (value << 16) | (res & 0xFFFF);
        else
            res = (res & 0xFFFF0000) | value;

        write32(location, offset & aligned_mask, res);
    }

    void write8(PCI::Location location, u32 offset, u8 value)
    {
        auto res = read32(location, offset & aligned_mask);

        u8 res_as_array[4];
        copy_memory(&res, res_as_array, sizeof(res));
        res_as_array[offset & 0b11] = value;
        copy_memory(res_as_array, &res, sizeof(res));

        write32(location, offset & aligned_mask, res);
    }

    virtual DynamicArray<PCI::DeviceInfo> list_all_devices() = 0;
};

}
