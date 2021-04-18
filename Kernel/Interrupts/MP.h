#pragma once

#include "Common/String.h"
#include "Core/Runtime.h"
#include "InterruptController.h"

namespace kernel {

// Aka MultiProcessor Specification
class MP {
public:
    static SMPData* parse();

    struct PCIIRQ {
        u8 device_number;
        u8 ioapic_id;
        u8 ioapic_pin;
        TriggerMode trigger_mode;
        Polarity polarity;
    };

    static Optional<PCIIRQ> try_deduce_pci_irq_number(u8 bus, u8 device);

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
            FAILED_ASSERTION("unknown entry type");
        }
    }

    struct PACKED ConfigurationTable {
        char signature[4];
        u16 length;
        u8 specification_revision;
        u8 checksum;
        char oem_id[8];
        char product_id[12];
        u32 oem_table_pointer;
        u16 oem_table_size;
        u16 entry_count;
        u32 lapic_address;
        u16 extended_table_length;
        u8 extended_table_checksum;
        u8 reserved;
    };

    struct PACKED FloatingPointer {
        char signature[4];
        u32 configuration_table_pointer;
        u8 length;
        u8 specification_revision;
        u8 checksum;
        u8 default_configuration;
        u32 features;
    };

    struct PACKED ProcessorEntry {
        enum class Flags : u8 {
            MUST_BE_IGNORED = 0,
            OK = 1,

            AP = 0,
            BSP = 2
        };

        friend bool operator&(Flags l, Flags r) { return static_cast<u8>(l) & static_cast<u8>(r); }

        EntryType type;
        u8 lapic_id;
        u8 lapic_version;
        Flags flags;
        u32 signature;
        u32 feature_flags;
        u64 reserved;
    };

    struct PACKED IOAPICEntry {
        enum class Flags : u8 {
            MUST_BE_IGNORED = 0,
            OK = 1,
        };

        friend bool operator&(Flags l, Flags r) { return static_cast<u8>(l) & static_cast<u8>(r); }

        EntryType type;
        u8 id;
        u8 version;
        Flags flags;
        u32 ioapic_address;
    };

    struct PACKED InterruptEntry {
        enum class Type : u8 {
            INT = 0,
            NMI = 1,
            SMI = 2,
            ExtINT = 3
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

        EntryType type;
        Type interrupt_type;
        Polarity polarity : 2;
        TriggerMode trigger_mode : 2;
        u16 reserved : 12;
        u8 source_bus_id;
        u8 source_bus_irq;

        union {
            u8 destination_ioapic_id;
            u8 destination_lapic_id;
        };

        union {
            u8 destination_ioapic_pin;
            u8 destination_lapic_lint;
        };
    };

    struct PACKED BusEntry {
        EntryType type;
        u8 id;
        char type_string[6];
    };

private:
    static FloatingPointer* find_floating_pointer_table();
    static SMPData* parse_configuration_table(FloatingPointer*);

private:
    static FloatingPointer* s_floating_pointer;
};

}
