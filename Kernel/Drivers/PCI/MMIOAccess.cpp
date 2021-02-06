#include "MMIOAccess.h"
#include "ACPI/ACPI.h"

namespace kernel {

MMIOAccess::MMIOAccess()
{
    auto mcfg_info = ACPI::the().get_table_info("MCFG"_sv);
    auto mcfg_mapping = TypedMapping<ACPI::SDTHeader>::create("MCFG"_sv, mcfg_info->physical_address, mcfg_info->length);
    Address mcfg = mcfg_mapping.get();
    Address mcfg_end = mcfg + mcfg_info->length;

    static constexpr size_t mcfg_reserved_field_size = 8;
    mcfg += mcfg_reserved_field_size + sizeof(ACPI::SDTHeader);

    while (mcfg < mcfg_end) {
        auto* entry = mcfg.as_pointer<MCFGEntry>();
        log() << "MCFG config space @ " << format::as_hex << entry->physical_base << " bus " << entry->start_pci_bus_number << " -> " << entry->end_pci_bus_number;

        m_config_spaces.append({ static_cast<Address::underlying_pointer_type>(entry->physical_base),
                                 entry->pci_segment_group_number,
                                 BasicRange<u8>::from_two_pointers(entry->start_pci_bus_number, entry->end_pci_bus_number + 1)});

        mcfg += MCFGEntry::size;
    }
}

}
