#pragma once

#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {

class CPU {
public:
    static void initialize();

private:
    static Address find_string_in_range(Address begin, Address end, size_t step, StringView string);

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
            case EntryType::PROCESSOR: return 20;
            case EntryType::BUS:
            case EntryType::IO_APIC:
            case EntryType::IO_INTERRUPT_ASSIGNMENT:
            case EntryType::LOCAL_INTERRUPT_ASSIGNMENT: return 8;
            default: ASSERT(false);
            }
        }

        static StringView entry_type_to_string(EntryType t)
        {
            switch (t) {
            case EntryType::PROCESSOR: return "Processor"_sv;
            case EntryType::BUS: return "Bus"_sv;
            case EntryType::IO_APIC: return "I/O APIC"_sv;
            case EntryType::IO_INTERRUPT_ASSIGNMENT: return "I/O interrupt assignment"_sv;
            case EntryType::LOCAL_INTERRUPT_ASSIGNMENT: return "Local interrupt assignment"_sv;
            default: return "Unknown"_sv;
            }
        }

        struct ConfigurationTable {
            char    signature[4];
            u16     length;
            u8      specification_revision;
            u8      checksum;
            char    oem_id[8];
            char    product_id[12];
            Address oem_table_pointer;
            u16     oem_table_size;
            u16     entry_count;
            Address local_apic_pointer;
            u16     extended_table_length;
            u8      extended_table_checksum;
            u8      reserved;
        };

        struct FloatingPointer {
            char                signature[4];
            ConfigurationTable* configuration_table_pointer;
            u8                  length;
            u8                  specification_revision;
            u8                  checksum;
            u8                  default_configuration;
            u32                 features;
        };

        struct ProcessorEntry {
            enum class Flags : u8 {
                MUST_BE_IGNORED = 0,
                OK              = 1,

                AP  = 0,
                BSP = 2
            };

            friend bool operator&(Flags l, Flags r) { return static_cast<u8>(l) & static_cast<u8>(r); }

            EntryType type;
            u8        local_apic_id;
            u8        local_apic_version;
            Flags     flags;
            u32       signature;
            u32       feature_flags;
            u64       reserved;
        };

        struct IOAPICEntry {
            enum class Flags : u8 {
                MUST_BE_IGNORED = 0,
                OK              = 1,
            };

            friend bool operator&(Flags l, Flags r) { return static_cast<u8>(l) & static_cast<u8>(r); }

            EntryType type;
            u8        id;
            u8        version;
            Flags     flags;
            Address   io_apic_pointer;
        };

        static void find_floating_pointer_table();
        static void parse_configuration_table();

        static FloatingPointer* s_floating_pointer;
    };
};
}
