#pragma once

#include "Access.h"
#include "Common/DynamicArray.h"
#include "Memory/MemoryManager.h"

namespace kernel {

class MMIOAccess : public Access
{
public:
    MMIOAccess();

    struct ConfigurationSpaceInfo {
        Address physical_base;
        u16 segment_group_number;
        u8 first_bus;
        u8 last_bus;
    };

    struct MCFGEntry {
        u64 physical_base;
        u16 pci_segment_group_number;
        u8 start_pci_bus_number;
        u8 end_pci_bus_number;
        u32 reserved;

        static constexpr size_t size = 16;
    };

    static constexpr size_t configuration_space_size = 4096;

    class DeviceEnumerator {
    public:
        DeviceEnumerator();

        DynamicArray<PCI::DeviceInfo>& enumerate_all(const DynamicArray<ConfigurationSpaceInfo>&);

#ifdef ULTRA_32
        ~DeviceEnumerator() { MemoryManager::the().free_virtual_region(*m_view_region); }
#endif

    private:
        void enumerate_function(u8 bus, u8 device, u8 function);
        void enumerate_device(u8 bus, u8 device);
        void enumerate_bus(u8 bus);

        Address virtual_base() const;
        void remap_base_for_device(PCI::Location);

        u32 read32(u32 offset);
        u16 read16(u32 offset);
        u8 read8(u32 offset);

        static MMIOAccess& access() { return static_cast<MMIOAccess&>(PCI::access()); }

    private:
#ifdef ULTRA_32
        MemoryManager::VR m_view_region;
#elif defined(ULTRA_64)
        Address m_current_virtual_base;
#endif
        DynamicArray<PCI::DeviceInfo> m_found_devices;
        u16 m_current_segment { 0 };
    };

    u32 read32(PCI::Location, u32 offset) override;
    DynamicArray<PCI::DeviceInfo> list_all_devices() override;

    static_assert(sizeof(MCFGEntry) == MCFGEntry::size, "Incorrect size of MCFG entry");

private:
    Address physical_base_for(PCI::Location) const;

private:
    DynamicArray<ConfigurationSpaceInfo> m_config_spaces;
};

}
