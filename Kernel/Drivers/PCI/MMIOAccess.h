#pragma once

#include "Access.h"
#include "Memory/Range.h"
#include "Common/DynamicArray.h"

namespace kernel {

class MMIOAccess : public Access
{
    MAKE_SINGLETON_INHERITABLE(Access, MMIOAccess);
public:
    struct ConfigurationSpaceInfo {
        Address physical_base;
        u16 segment_group_number;
        BasicRange<u8> bus_range;
    };

    struct MCFGEntry {
        u64 physical_base;
        u16 pci_segment_group_number;
        u8 start_pci_bus_number;
        u8 end_pci_bus_number;
        u32 reserved;

        static constexpr size_t size = 16;
    };

    static_assert (sizeof(MCFGEntry) == MCFGEntry::size, "Incorrect size of MCFG entry");

private:
    DynamicArray<ConfigurationSpaceInfo> m_config_spaces;
};

}
