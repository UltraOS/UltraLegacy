#pragma once

#include "Common/Macros.h"
#include "Common/Map.h"
#include "Common/Types.h"
#include "Memory/TypedMapping.h"

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

    const DynamicArray<TableInfo>& tables() const { return m_tables; }

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
