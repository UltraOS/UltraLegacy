#include "MMIOAccess.h"
#include "ACPI/ACPI.h"
#include "Memory/NonOwningVirtualRegion.h"

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
                                 entry->start_pci_bus_number,
                                 entry->end_pci_bus_number });

        mcfg += MCFGEntry::size;
    }
}

Address MMIOAccess::physical_base_for(PCI::Location location) const
{
    auto space = linear_search(m_config_spaces.begin(), m_config_spaces.end(), location,
                               [] (const ConfigurationSpaceInfo& config_info, const PCI::Location& location)
                               {
                                   return config_info.segment_group_number == location.segment &&
                                          location.bus >= config_info.first_bus && location.bus <= config_info.last_bus;
                               });

    if (space == m_config_spaces.end()) {
        String error_string;
        error_string << "PCIe: failed to find configuration space for device " << location;
        runtime::panic(error_string.c_string());
    }

    auto physical_base = space->physical_base;
    auto rebased_bus = location.bus - space->first_bus;

    physical_base += rebased_bus << 20 | location.device << 15 | location.function << 12;

    return physical_base;
}

u32 MMIOAccess::read32(PCI::Location location, u32 offset)
{
    return *MemoryManager::physical_to_virtual(physical_base_for(location) + offset).as_pointer<volatile u32>();
}

u32 MMIOAccess::DeviceEnumerator::read32(u32 offset)
{
    return *Address(virtual_base() + offset).as_pointer<volatile u32>();
}

u16 MMIOAccess::DeviceEnumerator::read16(u32 offset)
{
    return *Address(virtual_base() + offset).as_pointer<volatile u16>();
}

u8 MMIOAccess::DeviceEnumerator::read8(u32 offset)
{
    return *Address(virtual_base() + offset).as_pointer<volatile u8>();
}

MMIOAccess::DeviceEnumerator::DeviceEnumerator()
{
#ifdef ULTRA_32
    m_view_region = MemoryManager::the().allocate_kernel_non_owning("PCIe", { Address(), configuration_space_size });
#endif
}

Address MMIOAccess::DeviceEnumerator::virtual_base() const
{
#ifdef ULTRA_32
    return m_view_region->virtual_range().begin();
#elif defined(ULTRA_64)
    return m_current_virtual_base;
#endif
}

void MMIOAccess::DeviceEnumerator::remap_base_for_device(PCI::Location location)
{
    auto physical_base = access().physical_base_for(location);
#ifdef ULTRA_32
    auto& region = static_cast<NonOwningVirtualRegion&>(*m_view_region);
    region.switch_physical_range_and_remap({physical_base , configuration_space_size });
#elif defined(ULTRA_64)
    m_current_virtual_base = MemoryManager::physical_to_virtual(physical_base);
#endif
}

DynamicArray<PCI::DeviceInfo>& MMIOAccess::DeviceEnumerator::enumerate_all(const DynamicArray<ConfigurationSpaceInfo>& config_spaces)
{
    for (auto& space : config_spaces) {
        if (space.first_bus != 0)
            continue;

        remap_base_for_device({ 0, 0, 0, 0});

        auto is_multifunction = read8(PCI::header_type_offset) & PCI::multifunction_bit;
        ASSERT(is_multifunction == false);

        m_current_segment = space.segment_group_number;
        enumerate_bus(0);
    }

    return m_found_devices;
}

void MMIOAccess::DeviceEnumerator::enumerate_function(u8 bus, u8 device, u8 function) {
    PCI::Location location { m_current_segment, bus, device, function };
    remap_base_for_device(location);

    u8 device_class = read8(PCI::class_offset);
    u8 device_sub_class = read8(PCI::subclass_offset);

    if (device_class == PCI::bridge_device_class && device_sub_class == PCI::pci_to_pci_bridge_subclass) {
        auto subordinate_bus = read8(PCI::subordinate_bus_offset);
        log() << "PCIe: detected subordinate bus " << subordinate_bus;
        enumerate_bus(subordinate_bus);
    } else {
        PCI::DeviceInfo device;
        device.location = location;
        device.device_id = read16(PCI::device_id_offset);
        device.vendor_id = read16(PCI::vendor_id_offset);
        m_found_devices.append(device);
        log() << "PCIe: detected device " << device;
    }
};

void MMIOAccess::DeviceEnumerator::enumerate_device(u8 bus, u8 device) {
    u8 last_function = 1;

    remap_base_for_device({ m_current_segment, bus, device, 0 });

    auto vendor_id = read16(PCI::vendor_id_offset);

    if (vendor_id == PCI::invalid_vendor)
        return;

    enumerate_function(bus, device, 0);
    auto is_multifunction = read8(PCI::header_type_offset) & PCI::multifunction_bit;

    if (is_multifunction)
        last_function = 8;

    for (u8 function = 1; function < last_function; ++function) {
        remap_base_for_device({ m_current_segment, bus, device, function });

        if (read16(PCI::vendor_id_offset) != PCI::invalid_vendor)
            enumerate_function(bus, device, function);
    }
};

void MMIOAccess::DeviceEnumerator::enumerate_bus(u8 bus) {
    for (u8 device = 0; device < 32; ++device)
        enumerate_device(bus, device);
}

DynamicArray<PCI::DeviceInfo> MMIOAccess::list_all_devices()
{
    DeviceEnumerator enumerator;

    return move(enumerator.enumerate_all(m_config_spaces));
}

}
