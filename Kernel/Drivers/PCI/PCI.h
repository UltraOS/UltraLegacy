#pragma once

#include "Common/Types.h"
#include "Common/DynamicArray.h"
#include "Common/String.h"

namespace kernel {

class Access;

class PCI {
public:
    static constexpr u32 vendor_id_offset = 0x0;
    static constexpr u32 device_id_offset = 0x2;
    static constexpr u16 invalid_vendor = 0xFFFF;
    static constexpr u32 header_type_offset = 0xE;
    static constexpr u32 multifunction_bit = SET_BIT(7);
    static constexpr u32 subclass_offset = 0xA;
    static constexpr u32 class_offset = 0xB;
    static constexpr u32 subordinate_bus_offset = 0x1A;

    static constexpr u8 bridge_device_class = 0x6;
    static constexpr u8 pci_to_pci_bridge_subclass = 0x4;

    struct Location {
        u16 segment;
        u8 bus;
        u8 device;
        u8 function;

        template <typename LoggerT>
        friend LoggerT&& operator<<(LoggerT&& logger, const Location& location)
        {
            logger << format::as_dec << location.segment << ":" << location.bus << ":" << location.device << ":" << location.function;
            return logger;
        }
    };

    struct DeviceInfo {
        Location location;
        u16 vendor_id;
        u16 device_id;

        template <typename LoggerT>
        friend LoggerT&& operator<<(LoggerT&& logger, const DeviceInfo& device)
        {
            logger << format::as_hex <<  device.vendor_id << ":" << device.device_id << " @ " << device.location;
            return logger;
        }
    };

    void detect_all();

    static PCI& the()
    {
        return s_instance;
    }

    static Access& access();

private:
    DynamicArray<DeviceInfo> m_devices;

    static Access* s_access;
    static PCI s_instance;
};

}
