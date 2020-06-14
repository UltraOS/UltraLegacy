#include "MemoryManager.h"
#include "PhysicalRegion.h"

namespace kernel {

    PhysicalRegion::PhysicalRegion(ptr_t starting_address)
        : m_starting_address(starting_address)
    {
    }

    void PhysicalRegion::expand()
    {
        ++m_page_count;
    }

    void PhysicalRegion::seal()
    {
        // assert(bitmap.size() == 0)
        // bitmap.set_size(m_page_count);
    }
}
