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
    static constexpr u32 status_register_offset = 0x6;
    static constexpr u32 base_bar_offset = 0x10;
    static constexpr u16 invalid_vendor = 0xFFFF;
    static constexpr u32 header_type_offset = 0xE;
    static constexpr u32 programming_interface_offset = 0x9;
    static constexpr u32 subclass_offset = 0xA;
    static constexpr u32 class_offset = 0xB;
    static constexpr u32 secondary_bus_offset = 0x19;
    static constexpr u32 subordinate_bus_offset = 0x1A;
    static constexpr u32 capability_pointer_offset = 0x34;
    static constexpr u32 legacy_irq_line_offset = 0x3C;

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

        struct Capability {
            enum class ID : u8 {
                PCI_POWER_MANAGEMENT_INTERFACE = 0x01,
                ACCELERATED_GRAPHICS_PORT = 0x02,
                VITAL_PRODUCT_DATA = 0x03,
                SLOT_IDENTIFICATION = 0x04,
                MESSAGE_SIGNALED_INTERRUPTS = 0x05,
                COMPACT_PCI_HOT_SWAP = 0x06,
                PCI_X = 0x07,
                HYPER_TRANSPORT = 0x08,
                VENDOR_SPECIFIC = 0x09,
                DEBUG_PORT = 0x0A,
                COMPACT_PCI = 0x0B,
                PCI_HOT_PLUG = 0x0C,
                ACCELERATED_GRAPHICS_PORT_8X = 0x0E,
                SECURE_DEVICE = 0x0F,
                PCI_EXPRESS = 0x10,
                MESSAGE_SIGNALED_INTERRUPTS_EXTENDED = 0x11,
                SATA_DATA_CONFIGURATION = 0x12,
                ADVANCED_FEATURES = 0x13,
                ENHANCED_ALLOCATION = 0x14,
                FLATTENING_PORTAL_BRIDGE = 0x15
            } id;

            StringView id_to_string() const
            {
                switch (id) {
                case ID::PCI_POWER_MANAGEMENT_INTERFACE:
                    return "PCI Power Management Interface"_sv;
                case ID::ACCELERATED_GRAPHICS_PORT:
                    return "Accelerated Graphics Port"_sv;
                case ID::VITAL_PRODUCT_DATA:
                    return "Vital Product Data"_sv;
                case ID::SLOT_IDENTIFICATION:
                    return "Slot Identification"_sv;
                case ID::MESSAGE_SIGNALED_INTERRUPTS:
                    return "Message Signaled Interrupts"_sv;
                case ID::COMPACT_PCI_HOT_SWAP:
                    return "Compact PCI Hot Swap"_sv;
                case ID::PCI_X:
                    return "PCI-X"_sv;
                case ID::HYPER_TRANSPORT:
                    return "HyperTransport"_sv;
                case ID::VENDOR_SPECIFIC:
                    return "<Vendor Specific>"_sv;
                case ID::DEBUG_PORT:
                    return "Debug Port"_sv;
                case ID::COMPACT_PCI:
                    return "Compact PCI"_sv;
                case ID::PCI_HOT_PLUG:
                    return "PCI Hot Plug"_sv;
                case ID::ACCELERATED_GRAPHICS_PORT_8X:
                    return "Accelerated Graphics Port 8x"_sv;
                case ID::SECURE_DEVICE:
                    return "Secure Device"_sv;
                case ID::PCI_EXPRESS:
                    return "PCI Express"_sv;
                case ID::MESSAGE_SIGNALED_INTERRUPTS_EXTENDED:
                    return "Message Signaled Interrupts Extended";
                case ID::SATA_DATA_CONFIGURATION:
                    return "Sata Data/Index Configuration"_sv;
                case ID::ADVANCED_FEATURES:
                    return "Advanced Features"_sv;
                case ID::ENHANCED_ALLOCATION:
                    return "Enhanced Allocation"_sv;
                case ID::FLATTENING_PORTAL_BRIDGE:
                    return "Flattening Portal Bridge"_sv;
                default:
                    return "Unknown"_sv;
                }
            }

            u8 offset;
        };

        DynamicArray<Capability> capabilities;

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

    class Device {
    public:
        struct BAR {
            enum class Type {
                IO,
                MEMORY32,
                MEMORY64,
                NOT_PRESENT
            } type { Type::NOT_PRESENT };

            size_t span { 0 };
#ifdef ULTRA_32
            size_t span_upper { 0 };
#endif

            Address address { nullptr };
#ifdef ULTRA_32
            Address address_upper { nullptr };
#endif

            [[nodiscard]] bool is_memory() const
            {
                return type == Type::MEMORY32 || type == Type::MEMORY64;
            }
            [[nodiscard]] bool is_present() const { return type != Type::NOT_PRESENT; }
            [[nodiscard]] bool is_io() const { return type == Type::IO; }
        };

        Device(const DeviceInfo&);

        [[nodiscard]] const DeviceInfo& pci_device_info() const { return m_info; }
        [[nodiscard]] const Location& location() const { return m_info.location; }

        BAR bar(u8);

#ifdef ULTRA_64
        bool can_use_bar(const BAR&)
        {
            return true;
        }
#elif defined(ULTRA_32)
        bool can_use_bar(const BAR&);
#endif

        enum class MemorySpaceMode {
            DISABLED,
            ENABLED,
            UNTOUCHED
        };
        enum class IOSpaceMode {
            DISABLED,
            ENABLED,
            UNTOUCHED
        };
        enum class BusMasterMode {
            DISABLED,
            ENABLED,
            UNTOUCHED
        };
        struct CommandRegister {
            MemorySpaceMode memory_space;
            IOSpaceMode io_space;
            BusMasterMode bus_master;
        };

        // returns the original command register values
        CommandRegister set_command_register(
            MemorySpaceMode = MemorySpaceMode::UNTOUCHED,
            IOSpaceMode = IOSpaceMode::UNTOUCHED,
            BusMasterMode = BusMasterMode::UNTOUCHED);

        void enable_msi(u16 vector);
        void clear_interrupts_disabled();

        u16 legacy_pci_irq_line();

    private:
        DeviceInfo m_info;
    };

private:
    DynamicArray<DeviceInfo> m_devices;

    static Access* s_access;
    static PCI s_instance;
    static DynamicArray<autodetect_handler_t>* s_autodetect_handlers;
};

}
