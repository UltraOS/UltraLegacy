#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class GenericPagingEntry {
public:
    enum Attributes : u16 {
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
        GLOBAL        = SET_BIT(8)
    };

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

#endif

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