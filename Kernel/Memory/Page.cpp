#include "Page.h"
#include "MemoryManager.h"

namespace kernel {

Page::Page(ptr_t physical_address) : m_physical_address(physical_address) { }

ptr_t Page::address() const
{
    return m_physical_address;
}

Page::~Page()
{
    MemoryManager::the().free_page(*this);
}
}
