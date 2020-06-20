#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel {

    class PageEntry
    {
    public:
        enum Attributes : u32
        {
            NOT_PRESENT   = 0,
            PRESENT       = SET_BIT(0),
            READ_ONLY     = 0,
            READ_WRITE    = SET_BIT(1),
            SUPERVISOR    = 0,
            USER          = SET_BIT(2),
            WRITE_THROUGH = SET_BIT(3),
            DO_NOT_CACHE  = SET_BIT(4),
            PAGE_4KB      = 0,
            PAGE_4MB      = SET_BIT(7),
            GLOBAL        = SET_BIT(8),
        };

        friend Attributes operator|(Attributes l, Attributes r)
        {
            return static_cast<Attributes>(static_cast<u32>(l) | static_cast<u32>(r));
        }

        friend Attributes operator&(Attributes l, Attributes r)
        {
            return static_cast<Attributes>(static_cast<u32>(l) & static_cast<u32>(r));
        }

        friend Attributes operator~(Attributes a)
        {
            return static_cast<Attributes>(~static_cast<u32>(a));
        }

        void set_present(bool setting)
        {
            if (setting)
                set_attributes(attributes() | PRESENT);
            else
                set_attributes(attributes() & ~PRESENT);
        }

        void make_global()
        {
            set_attributes(attributes() | GLOBAL);
        }

        void make_supervisor_present()
        {
            set_attributes(PRESENT | READ_WRITE | SUPERVISOR);
        }

        void make_user_present()
        {
            set_attributes(PRESENT | READ_WRITE | USER);
        }

        PageEntry& set_physical_address(ptr_t address)
        {
            m_physical_address = address >> 12;

            return *this;
        }

        PageEntry& set_attributes(Attributes attributes)
        {
            m_attributes = attributes;

            return *this;
        }

        ptr_t physical_address() const
        {
            return m_physical_address << 12;
        }

        Attributes attributes() const
        {
            return m_attributes;
        }
    private:
        Attributes m_attributes       : 12;
        ptr_t      m_physical_address : 20;
    };
}