#pragma once

#include "Common/Types.h"
#include "Common/RefPtr.h"
#include "Range.h"

namespace kernel {

class AddressSpace
{
public:
    static bool is_initialized() { return true; }
};

class VirtualRegion
{
public:
    VirtualRegion(Range range) : m_range(range) {}

    const Range& virtual_range() { return m_range; }

private:
    Range m_range;
};

class PrivateVirtualRegion : public VirtualRegion {
public:
    void preallocate_range(bool = true) { }
};

class MemoryManager {
public:
    static constexpr Address kernel_first_heap_block_base = static_cast<ptr_t>(0);
    static constexpr Address kernel_first_heap_block_size = static_cast<ptr_t>(4 * MB);

    static bool is_initialized() { return true; }

    RefPtr<VirtualRegion> allocate_kernel_private_anywhere(StringView, size_t size)
    {
        Range r(malloc(size), size);

        return RefPtr<VirtualRegion>::create(r);
    }

    static MemoryManager& the() { return fake_mm; }

private:
    static MemoryManager fake_mm;
};

inline MemoryManager MemoryManager::fake_mm;

}
