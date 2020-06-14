#include "MemoryManager.h"
#include "PhysicalRegion.h"
#include "Page.h"

namespace kernel {

    PhysicalRegion::PhysicalRegion(ptr_t starting_address, size_t length)
        : m_starting_address(starting_address),
          m_page_count(length / Page::size),
          m_free_page_count(m_page_count)
    {
    }
}
