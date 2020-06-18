#include "VirtualAllocator.h"

namespace kernel {

    VirtualAllocator::VirtualAllocator()
    {
    }

    VirtualAllocator::VirtualAllocator(ptr_t base, size_t length)
        : m_base(base), m_length(length)
    {
    }

    void VirtualAllocator::set_range(ptr_t base, size_t length)
    {
        m_base   = base;
        m_length = length;
    }
}
