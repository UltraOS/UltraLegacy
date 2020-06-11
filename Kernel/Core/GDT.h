#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel {
    class GDT
    {
    public:
        enum access_attributes : u8
        {
            NULL_SELECTOR  = 0,
            RING_0         = 0,
            TASK           = 0,
            PRESENT        = SET_BIT(7),
            RING_3         = SET_BIT(5) | SET_BIT(6),
            CODE_OR_DATA   = SET_BIT(4),
            EXECUTABLE     = SET_BIT(3),
            GROWS_DOWN     = SET_BIT(2),
            ALLOW_LOWER    = SET_BIT(2),
            READABLE       = SET_BIT(1),
            WRITABLE       = SET_BIT(1),
        };

        enum flag_attributes : u8
        {
            GRANULARITY_1B  = 0,
            GRANULARITY_4KB = SET_BIT(3),
            MODE_16_BIT     = 0,
            MODE_32_BIT     = SET_BIT(2),
        };

        friend access_attributes operator|(access_attributes l, access_attributes r)
        {
            return static_cast<access_attributes>(static_cast<u8>(l) | static_cast<u8>(r));
        }

        friend flag_attributes operator|(flag_attributes l, flag_attributes r)
        {
            return static_cast<flag_attributes>(static_cast<u8>(l) | static_cast<u8>(r));
        }

        // 1 NULL selector + 8 free
        static constexpr u16 entry_count = 9;

        void create_basic_descriptors();

        void install();

        void create_descriptor(u32 base, u32 size, access_attributes access, flag_attributes flags);

        static constexpr u16 kernel_code_selector() { return 0x8; }
        static constexpr u16 kernel_data_selector() { return 0x10; }

        static GDT& the();
    private:
        GDT();

        u16 m_active_entries { 0 };

        struct PACKED entry
        {
            u16 limit_lower;
            u16 base_lower;
            u8  base_middle;
            access_attributes access;
            u8  limit_upper       : 4;
            flag_attributes flags : 4;
            u8  base_upper;
        } m_entries[entry_count];

        entry& new_entry();

        struct PACKED pointer
        {
            u16     size;
            entry*  address;
        } m_pointer;

        static GDT s_instance;
    };
}
