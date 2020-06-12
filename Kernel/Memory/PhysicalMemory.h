#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel {

    struct PACKED PhysicalRange
    {
        u64  base_address;
        u64  length;

        enum : u32
        {
            FREE         = 1,
            RESERVED     = 2,
            RECLAIMABLE  = 3,
            NON_VOLATILE = 4,
            BAD          = 5
        } type;

        u32 extended_attributes;

        bool is_free()     const { return type == FREE; }
        bool is_reserved() const { return !is_free(); }

        const char* type_as_string() const
        {
            switch (type)
            {
            case FREE:
                return "free";
            case RESERVED:
                return "reserved";
            case RECLAIMABLE:
                return "reclaimable";
            case NON_VOLATILE:
                return "ACPI non-volatile";
            case BAD:
                return "bad";
            default:
                return "unknown";
            }
        }

        friend AutoLogger& operator<<(AutoLogger&& logger, const PhysicalRange& range)
        {
            logger << "PhysicalRange: start:" 
                   << format::as_hex
                   << range.base_address
                   << " size:" << range.length
                   << " type: " << range.type_as_string();

            return logger;
        }
    };

    struct PACKED MemoryMap
    {
        PhysicalRange* entries;
        u16 entry_count;

        PhysicalRange* begin() { return entries; }
        PhysicalRange* end()   { return entries + entry_count; }
    };
}
