#pragma once

#include "Common/Macros.h"
#include "Common/Map.h"
#include "Common/Types.h"
#include "Memory/TypedMapping.h"
#include "Interrupts/Utilities.h"

namespace kernel {

class ACPI {
public:
    struct PACKED RSDP {
        char signature[8];
        u8 checksum;
        char oemid[6];
        u8 revision;
        u32 rsdt_pointer;

        // ^ rev. 1 | rev. 2 v

        u32 length;
        u64 xsdt_pointer;
        u8 extended_checksum;
        u8 reserved[3];
    };

    struct PACKED SDTHeader {
        char signature[4];
        u32 length;
        u8 revision;
        u8 checksum;
        u8 oemid[6];
        u8 oem_table_id[8];
        u32 oem_revision;
        u32 creator_id;
        u32 creator_revision;
    };

    struct TableInfo {
        Address physical_address;
        size_t length;
        String name;
    };

    static constexpr size_t rsdp_revision_1_size = 20;
    static constexpr size_t rsdp_revision_2_size = 36;
    static constexpr size_t sdt_header_size = 36;

    static_assert(sizeof(RSDP) == rsdp_revision_2_size, "Incorrect RSDP size");
    static_assert(sizeof(SDTHeader) == sdt_header_size, "Incorrect SDTHeader size");

    static ACPI& the() { return s_instance; }

    void initialize();

    u8 rsdp_revision() const { return m_rsdp ? m_rsdp->revision : 0xFF; }

    TableInfo* get_table_info(StringView signature);
    const DynamicArray<TableInfo>& tables() const { return m_tables; }

    struct PACKED MADT {
        enum class Flags : u32 {
            PCAT_COMPAT = 1,
        };

        SDTHeader header;
        u32 lapic_address;
        Flags flags;

        enum class EntryType : u8 {
            LAPIC = 0x0,
            IOAPIC = 0x1,
            INTERRUPT_SOURCE_OVERRIDE = 0x2,
            NMI_SOURCE = 0x3,
            LAPIC_NMI = 0x4,
            LAPIC_ADDRESS_OVERRIDE = 0x5,
            IOSAPIC = 0x6,
            LOCAL_SAPIC = 0x7,
            PLATFORM_INTERRUPT_SOURCES = 0x8,
            X2APIC = 0x9,
            X2APIC_NMI = 0xA,
            GIC_CPU_INTERFACE = 0xB,
            GIC_DISTRIBUTOR = 0xC,
            GIC_MSI_FRAME = 0xD,
            GIC_REDISTRIBUTOR = 0xE,
            GIC_ITS = 0xF,
            MULTIPROCESSOR_WAKEUP = 0x10,
        };

        struct PACKED EntryHeader {
            MADT::EntryType type;
            u8 length;
        };

        struct PACKED LAPIC {
            enum class Flags : u32 {
                ENABLED = 1,
                ONLINE_CAPABLE = 2,
            };

            friend bool operator&(Flags l, Flags r) {
                return static_cast<u32>(l) & static_cast<u32>(r);
            }

            EntryHeader header;
            u8 acpi_uid;
            u8 id;
            Flags flags;

            bool should_be_ignored() const
            {
                return !((flags & Flags::ENABLED) ^ (flags & Flags::ONLINE_CAPABLE));
            }
        };

        struct PACKED IOAPIC {
            EntryHeader header;
            u8 id;
            u8 reserved;
            u32 address;
            u32 gsi_base;
        };

        struct PACKED InterruptSourceOverride {
            enum class Bus : u8 {
                ISA = 0,
            };

            EntryHeader header;
            Bus bus;
            u8 source;
            u32 gsi;
            Polarity polarity : 2;
            TriggerMode trigger_mode : 2;
            u16 reserved : 12;
        };

        static constexpr size_t size = sdt_header_size + 8;
        static constexpr size_t entry_header_size = 2;
        static constexpr size_t lapic_entry_size = 8;
        static constexpr size_t ioapic_entry_size = 12;
        static constexpr size_t interrupt_source_override_size = 10;
    };


    static_assert(sizeof(MADT) == MADT::size, "Incorrect size of MADT");
    static_assert(sizeof(MADT::EntryHeader) == MADT::entry_header_size, "Incorrect size of MADT entry");
    static_assert(sizeof(MADT::LAPIC) == MADT::lapic_entry_size, "Incorrect size of LAPIC");
    static_assert(sizeof(MADT::IOAPIC) == MADT::ioapic_entry_size, "Incorrect size of IOAPIC");
    static_assert(sizeof(MADT::InterruptSourceOverride) == MADT::interrupt_source_override_size, "Incorrect interrupt source override size");

    SMPData* generate_smp_data();

private:
    bool verify_checksum(Address virtual_address, size_t length);
    bool find_rsdp();
    void collect_all_sdts();

    size_t length_of_table_from_physical_address(Address physical);

private:
    RSDP* m_rsdp { nullptr };
    DynamicArray<TableInfo> m_tables;

    static ACPI s_instance;
};
}
