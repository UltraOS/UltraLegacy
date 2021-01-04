#pragma once

#include "Common/DynamicArray.h"
#include "Common/Macros.h"
#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {

class InterruptController {
    MAKE_INHERITABLE_SINGLETON(InterruptController) = default;

public:
    enum class Type {
        PIC,
        APIC,
    };

    static void discover_and_setup();

    static InterruptController& the();

    // TODO: this is a dumb way to detect smp support, redo this
    static bool supports_smp() { return s_smp_data != nullptr; }

    struct IRQInfo {
        u8 original_irq_index;
        u8 redirected_irq_index;
        bool is_edge;
        bool is_active_high;
    };

    struct SMPData {
        DynamicArray<u8> application_processor_apic_ids;
        u8 bootstrap_processor_apic_id;
        Address lapic_address;
        Address ioapic_address;
        DynamicArray<IRQInfo> irqs;
    };

    static const SMPData& smp_data()
    {
        ASSERT(s_smp_data != nullptr);

        return *s_smp_data;
    }

    static bool is_initialized() { return s_instance != nullptr; }

    virtual void end_of_interrupt(u8 request_number) = 0;

    virtual void clear_all() = 0;

    virtual void enable_irq(u8 index) = 0;
    virtual void disable_irq(u8 index) = 0;

    virtual bool is_spurious(u8 request_number) = 0;
    virtual void handle_spurious_irq(u8 request_number) = 0;

    virtual Type type() const = 0;

private:
    class MP {
    public:
        enum class EntryType : u8 {
            PROCESSOR,
            BUS,
            IO_APIC,
            IO_INTERRUPT_ASSIGNMENT,
            LOCAL_INTERRUPT_ASSIGNMENT,
        };

        static size_t sizeof_entry(EntryType t)
        {
            switch (t) {
            case EntryType::PROCESSOR:
                return 20;
            case EntryType::BUS:
            case EntryType::IO_APIC:
            case EntryType::IO_INTERRUPT_ASSIGNMENT:
            case EntryType::LOCAL_INTERRUPT_ASSIGNMENT:
                return 8;
            default:
                ASSERT(false);
            }
        }

        struct ConfigurationTable {
            char signature[4];
            u16 length;
            u8 specification_revision;
            u8 checksum;
            char oem_id[8];
            char product_id[12];
            Address32 oem_table_pointer;
            u16 oem_table_size;
            u16 entry_count;
            Address32 local_apic_pointer;
            u16 extended_table_length;
            u8 extended_table_checksum;
            u8 reserved;
        };

        struct FloatingPointer {
            char signature[4];
            Address32 configuration_table_pointer;
            u8 length;
            u8 specification_revision;
            u8 checksum;
            u8 default_configuration;
            u32 features;
        };

        struct ProcessorEntry {
            enum class Flags : u8 {
                MUST_BE_IGNORED = 0,
                OK = 1,

                AP = 0,
                BSP = 2
            };

            friend bool operator&(Flags l, Flags r) { return static_cast<u8>(l) & static_cast<u8>(r); }

            EntryType type;
            u8 local_apic_id;
            u8 local_apic_version;
            Flags flags;
            u32 signature;
            u32 feature_flags;
            u64 reserved;
        };

        struct IOAPICEntry {
            enum class Flags : u8 {
                MUST_BE_IGNORED = 0,
                OK = 1,
            };

            friend bool operator&(Flags l, Flags r) { return static_cast<u8>(l) & static_cast<u8>(r); }

            EntryType type;
            u8 id;
            u8 version;
            Flags flags;
            Address32 io_apic_pointer;
        };

        struct InterruptEntry {
            enum class Type : u8 {
                INT = 0,
                NMI = 1,
                SMI = 2,
                ExtINT = 3
            };

            enum class Polarity : u8 {
                CONFORMING = 0,
                ACTIVE_HIGH = 1,
                ACTIVE_LOW = 3
            };

            enum class TriggerMode : u8 {
                CONFORMING = 0,
                EDGE = 1,
                LEVEL = 3
            };

            static StringView to_string(Type t)
            {
                switch (t) {
                case Type::INT:
                    return "Vectored"_sv;
                case Type::NMI:
                    return "Nonmaskable"_sv;
                case Type::SMI:
                    return "System Management"_sv;
                case Type::ExtINT:
                    return "External"_sv;
                default:
                    return "Unknown"_sv;
                }
            }

            static StringView to_string(Polarity p)
            {
                switch (p) {
                case Polarity::CONFORMING:
                    return "Conforming"_sv;
                case Polarity::ACTIVE_HIGH:
                    return "Active High"_sv;
                case Polarity::ACTIVE_LOW:
                    return "Active Low"_sv;
                default:
                    return "Unknown"_sv;
                }
            }

            static StringView to_string(TriggerMode t)
            {
                switch (t) {
                case TriggerMode::CONFORMING:
                    return "Conforming"_sv;
                case TriggerMode::EDGE:
                    return "Edge"_sv;
                case TriggerMode::LEVEL:
                    return "Level"_sv;
                default:
                    return "Unknown"_sv;
                }
            }

            EntryType type;
            Type interrupt_type;
            Polarity polarity : 2;
            TriggerMode trigger_mode : 2;
            u16 reserved : 12;
            u8 source_bus_id;
            u8 source_bus_irq;
            u8 destination_ioapic_id;
            u8 destination_ioapic_pin;
        };

        struct BusEntry {
            EntryType type;
            u8 id;
            char type_string[6];
        };

        static FloatingPointer* find_floating_pointer_table();
        static void parse_configuration_table(FloatingPointer*);

        static FloatingPointer* s_floating_pointer;
    };

private:
    static SMPData* s_smp_data;
    static InterruptController* s_instance;
};
}
