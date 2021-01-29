#include "ACPI.h"
#include "Common/String.h"
#include "Memory/MemoryManager.h"
#include "Memory/NonOwningVirtualRegion.h"

namespace kernel {

ACPI ACPI::s_instance;

void ACPI::initialize()
{
    if (!find_rsdp()) {
        log() << "ACPI: couldn't find RSDP";
        return;
    }

    if (!verify_checksum(m_rsdp, m_rsdp->revision == 2 ? rsdp_revision_2_size : rsdp_revision_1_size)) {
        warning() << "ACPI: RSDP invalid checksum detected";
        return;
    }

    log() << "ACPI: RSDP revision " << m_rsdp->revision << " @ " << MemoryManager::virtual_to_physical(m_rsdp) << " checksum OK";
    collect_all_sdts();
}

bool ACPI::find_rsdp()
{
    static constexpr StringView rsdp_signature = "RSD PTR "_sv;
    static constexpr size_t rsdp_alignment = 16;

    static constexpr Address ebda_base = 0x80000;
    auto ebda_range = Range::from_two_pointers(
        MemoryManager::physical_to_virtual(ebda_base),
        MemoryManager::physical_to_virtual(ebda_base + 1 * KB));

    auto rsdp = rsdp_signature.find_in_range(ebda_range, rsdp_alignment);

    if (!rsdp.empty()) {
        m_rsdp = Address(rsdp.data()).as_pointer<RSDP>();
        return true;
    }

    static constexpr Address bios_base = 0xE0000;
    static constexpr Address bios_end = 0xFFFFF;
    auto bios_range = Range::from_two_pointers(
        MemoryManager::physical_to_virtual(bios_base),
        MemoryManager::physical_to_virtual(bios_end));

    rsdp = rsdp_signature.find_in_range(bios_range, rsdp_alignment);

    if (!rsdp.empty()) {
        m_rsdp = Address(rsdp.data()).as_pointer<RSDP>();
        return true;
    }

    return false;
}

size_t ACPI::length_of_table_from_physical_address(Address physical)
{
    return TypedMapping<SDTHeader>::create("ACPI"_sv, physical)->length;
}

void ACPI::collect_all_sdts()
{
    Address root_sdt_physical;
    StringView root_sdt_name;
    size_t pointer_stride;

    if (m_rsdp->revision == 2) {
        root_sdt_physical = static_cast<Address::underlying_pointer_type>(m_rsdp->xsdt_pointer);
        root_sdt_name = "XSDT"_sv;
        pointer_stride = 8;
    } else {
        root_sdt_physical = static_cast<Address::underlying_pointer_type>(m_rsdp->rsdt_pointer);
        root_sdt_name = "RSDT"_sv;
        pointer_stride = 4;
    }

    auto root_sdt_length = length_of_table_from_physical_address(root_sdt_physical);
    auto root_sdt_mapping = TypedMapping<SDTHeader>::create("ACPI"_sv, root_sdt_physical, root_sdt_length);
    Address root_sdt = root_sdt_mapping.get();

    if (!verify_checksum(root_sdt, root_sdt_length)) {
        log() << root_sdt_name << " invalid checksum detected";
        return;
    }

    log() << "ACPI: " << root_sdt_name << " checksum OK";

    root_sdt += sizeof(SDTHeader);
    auto root_sdt_end = root_sdt + (root_sdt_length - sizeof(SDTHeader));
    m_tables.reserve((root_sdt_end - root_sdt) /  pointer_stride);

    for (; root_sdt < root_sdt_end; root_sdt += pointer_stride) {
        Address table_address;

        if (pointer_stride == 4)
            table_address = Address(*root_sdt.as_pointer<u32>());
        else
            table_address = Address(*root_sdt.as_pointer<u64>());

        auto length_of_table = length_of_table_from_physical_address(table_address);
        auto this_table_mapping = TypedMapping<SDTHeader>::create("ACPI"_sv, table_address, length_of_table);
        auto* this_table = this_table_mapping.get();

        bool checksum_ok = verify_checksum(this_table, length_of_table);
        auto signature_string = StringView(this_table->signature, 4);

        if (checksum_ok) {
            log() << "ACPI: table " << signature_string << " @ " << table_address << " checksum OK";
        } else {
            warning() << "ACPI: table " << signature_string << " @ " << table_address << " invalid checksum detected";
            continue;
        }

        m_tables.append({ table_address, length_of_table, signature_string });
    }
}

bool ACPI::verify_checksum(Address virtual_address, size_t length)
{
    u8* bytes = virtual_address.as_pointer<u8>();
    u8 sum = 0;

    for (size_t i = 0; i < length; ++i)
        sum += bytes[i];

    return sum == 0;
}

}
