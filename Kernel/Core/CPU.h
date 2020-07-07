#pragma once

#include "Common/Types.h"
#include "Common/String.h"

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

            static StringView entry_type_to_string(EntryType t)
            {
                switch (t)
                {
                case EntryType::PROCESSOR: return "Processor";
                case EntryType::BUS: return "Bus";
                case EntryType::IO_APIC: return "I/O APIC";
                case EntryType::IO_INTERRUPT_ASSIGNMENT: return "I/O interrupt assignment";
                case EntryType::LOCAL_INTERRUPT_ASSIGNMENT: return "Local interrupt assignment";
                default: return "Unknown";
                }
            }

            struct ConfigurationTable {
                char signature[4];
                u16 length;
                u8 specification_revision;
                u8 checksum;
                char oem_id[8];
                char product_id[12];
                Address oem_table_pointer;
                u16 oem_table_size;
                u16 entry_count;
                Address local_apic_pointer;
                u16 extended_table_length;
                u8 extended_table_checksum;
                u8 reserved;
            };

            struct FloatingPointer {
                char signature[4];
                ConfigurationTable* configuration_table_pointer;
                u8 length;
                u8 specification_revision;
                u8 checksum;
                u8 default_configuration;
                u32 features;
            };

            static void find_floating_pointer_table();
            static void parse_configuration_table();

            static FloatingPointer* s_floating_pointer;
        };
    };
}
