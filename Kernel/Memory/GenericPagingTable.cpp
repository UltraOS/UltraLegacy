#include "GenericPagingTable.h"
#include "MemoryManager.h"

namespace kernel {

#ifdef ULTRA_64
Address GenericPagingTable::accessible_address_of(size_t index)
{
    return MemoryManager::physical_memory_base + entry_at(index).physical_address();
}
#endif
}
