#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/Runtime.h"

namespace kernel {

class GenericPagingEntry {
public:
    // clang-format off
    enum Attributes : u16 {
        NOT_PRESENT   = 0,
        PRESENT       = SET_BIT(0),
        READ_ONLY     = 0,
        READ_WRITE    = SET_BIT(1),
        SUPERVISOR    = 0,
        USER          = SET_BIT(2),

        WRITE_THROUGH = SET_BIT(3),
        PWT_BIT       = SET_BIT(3),

        DO_NOT_CACHE  = SET_BIT(4),
        PCD_BIT       = SET_BIT(5),

        PAT_BIT_4K    = SET_BIT(7),

        PAGE_4KB      = 0,
#ifdef ULTRA_32
        PAGE_4MB      = SET_BIT(7),
#elif defined(ULTRA_64)
        PAGE_2MB      = SET_BIT(7),
        PAGE_1GB      = SET_BIT(7),
#endif
        GLOBAL        = SET_BIT(8),
    };
    // clang-format on

    friend Attributes operator|(Attributes l, Attributes r)
    {
        return static_cast<Attributes>(static_cast<u16>(l) | static_cast<u16>(r));
    }

    friend Attributes operator&(Attributes l, Attributes r)
    {
        return static_cast<Attributes>(static_cast<u16>(l) & static_cast<u16>(r));
    }

    friend Attributes operator~(Attributes a) { return static_cast<Attributes>(~static_cast<u16>(a)); }

    void set_present(bool setting)
    {
        if (setting)
            set_attributes(attributes() | PRESENT);
        else
            set_attributes(attributes() & ~PRESENT);
    }

    bool is_present() { return attributes() & PRESENT; }

#ifdef ULTRA_64

    void set_executable(bool setting) { m_no_execute = setting; }

    bool is_executable() { return !m_no_execute; }

    void set_huge(bool setting)
    {
        if (setting)
            set_attributes(attributes() | PAGE_2MB);
        else
            set_attributes(attributes() & ~PAGE_2MB);
    }

#endif

    void set_pat_index(u8 index, bool is_4k_page)
    {
        ASSERT(index < 8);

        if (index & 1) {
            m_attributes = m_attributes | Attributes::PWT_BIT;
        } else {
            m_attributes = m_attributes & ~(Attributes::PWT_BIT);
        }

        if (index & 2) {
            m_attributes = m_attributes | Attributes::PCD_BIT;
        } else {
            m_attributes = m_attributes & ~Attributes::PCD_BIT;
        }

        if (index & 4) {
            if (is_4k_page) {
                m_attributes = m_attributes | Attributes::PAT_BIT_4K;
            } else {
                // bit 0 is actually PAT
                m_physical_address |= 1;
            }
        } else {
            if (is_4k_page) {
                m_attributes = m_attributes & ~Attributes::PAT_BIT_4K;
            } else {
                // bit 0 is actually PAT
                m_physical_address &= ~static_cast<decltype(m_physical_address)>(1);
            }
        }
    }

    void make_global() { set_attributes(attributes() | GLOBAL); }

    void make_supervisor_present() { set_attributes(PRESENT | READ_WRITE | SUPERVISOR); }

    void make_user_present() { set_attributes(PRESENT | READ_WRITE | USER); }

    GenericPagingEntry& set_physical_address(Address address)
    {
        m_physical_address = address >> 12;

        return *this;
    }

    GenericPagingEntry& set_attributes(Attributes attributes)
    {
        m_attributes = attributes;

        return *this;
    }

    Address physical_address() const { return m_physical_address << 12; }

    Attributes attributes() const { return m_attributes; }

private:
    Attributes m_attributes : 12;

#ifdef ULTRA_32
    ptr_t m_physical_address : 20;
#elif defined(ULTRA_64)
    ptr_t m_physical_address : 40;
    u16   m_unused : 11;
    bool  m_no_execute : 1;
#endif
};

static_assert(sizeof(GenericPagingEntry) == sizeof(ptr_t));

}
