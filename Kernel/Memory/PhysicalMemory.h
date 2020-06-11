#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"


namespace kernel {

    enum class region_type : u32
    {
        FREE        = 1,
        RESERVED    = 2,
        RECLAIMABLE = 3,
        NVS         = 4,
        BAD         = 5
    };

    struct PACKED MemoryRange
    {
        u64 base_address;
        u64 length;
        region_type type;
        u32 extended_attributes;
    };

    struct PACKED MemoryMap
    {
        MemoryRange* entries;
        u16 entry_count;

        MemoryRange* begin() { return entries; }
        MemoryRange* end()   { return entries + entry_count; }
    };
}
