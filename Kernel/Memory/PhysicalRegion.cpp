#include "MemoryManager.h"
#include "PhysicalRegion.h"
#include "Page.h"

namespace kernel {

    PhysicalRegion::PhysicalRegion(ptr_t starting_address, size_t length)
        : m_starting_address(starting_address)
    {
        m_allocation_map.set_size(length / Page::size);
    }
}
