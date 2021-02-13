#pragma once

#include "Common/DynamicArray.h"
#include "Common/String.h"
#include "Common/Types.h"
#include "Drivers/Device.h"

namespace kernel {

class Access;

class PCI {
public:
    static constexpr u32 vendor_id_offset = 0x0;
    static constexpr u32 device_id_offset = 0x2;
    static constexpr u32 command_register_offset = 0x4;
    static constexpr u32 base_bar_offset = 0x10;
    static constexpr u16 invalid_vendor = 0xFFFF;
    static constexpr u32 header_type_offset = 0xE;
    static constexpr u32 programming_interface_offset = 0x9;
    static constexpr u32 subclass_offset = 0xA;
    static constexpr u32 class_offset = 0xB;
    static constexpr u32 subordinate_bus_offset = 0x1A;

    static constexpr u8 bridge_device_class = 0x6;
    static constexpr u8 pci_to_pci_bridge_subclass = 0x4;

    static constexpr u32 register_width = 4;

    static constexpr u32 multifunction_bit = SET_BIT(7);

    struct Location {
        u16 segment;
        u8 bus;
        u8 device;
        u8 function;

        friend bool operator==(const Location& l, const Location& r)
        {
            return l.segment == r.segment && l.bus == r.bus && l.device == r.device && l.function == r.function;
        }

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
        u8 class_code;
        u8 subclass_code;
        u8 programming_interface;

        template <typename LoggerT>
        friend LoggerT&& operator<<(LoggerT&& logger, const DeviceInfo& device)
        {
            logger << format::as_hex << device.vendor_id << ":" << device.device_id << " @ " << device.location;
            return logger;
        }
    };

    void collect_all_devices();
    void initialize_supported();

    static PCI& the()
    {
        return s_instance;
    }

    static Access& access();

    const DynamicArray<DeviceInfo>& devices() const { return m_devices; }

    using autodetect_handler_t = void (*)(const DynamicArray<PCI::DeviceInfo>&);
    static void register_autodetect_handler(autodetect_handler_t);

#define AUTO_DETECT_PCI(DEVICE)                                                   \
    class AutoDetector {                                                          \
    public:                                                                       \
        AutoDetector() { PCI::register_autodetect_handler(&DEVICE::autodetect); } \
    } inline static s_auto_detector;

    class Device : public ::kernel::Device {
    public:
        struct BAR {
            enum class Type {
                IO,
                MEMORY
            } type;

            Address address;
        };

        Device(Location);

        const Location& location() const { return m_location; }

        BAR bar(u8);

        void make_bus_master();
        void enable_memory_space();
        void enable_io_space();
        void clear_interrupts_disabled();

    private:
        Location m_location;
    };

private:
    DynamicArray<DeviceInfo> m_devices;

    static Access* s_access;
    static PCI s_instance;
    static DynamicArray<autodetect_handler_t>* s_autodetect_handlers;
};

}
