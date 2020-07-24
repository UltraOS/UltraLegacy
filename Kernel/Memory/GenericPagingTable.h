#pragma once

#include "GenericPagingEntry.h"

namespace kernel {

class GenericPagingTable {
public:
    using Entry = GenericPagingEntry;

#ifdef ULTRA_32
    static constexpr size_t  entry_count = 1024;
#elif defined(ULTRA_64)
    static constexpr size_t entry_count  = 512;
#endif

    static constexpr size_t entry_size = sizeof(Address);
    static constexpr size_t size       = entry_count * entry_size;

    Entry& entry_at(size_t index) { return m_entries[index]; }

protected:
#ifdef ULTRA_64
    Address accessible_address_of(size_t index);
#endif

protected:
    Entry m_entries[entry_count];
};
}
