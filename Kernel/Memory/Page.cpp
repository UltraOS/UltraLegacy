#include "Page.h"
#include "MemoryManager.h"

namespace kernel {

Page::Page(Address physical_address)
    : m_physical_address(physical_address)
{
}

Address Page::address() const
{
    return m_physical_address;
}

Page::~Page()
{
    MemoryManager::the().free_page(*this);
}
}
