#pragma once

#include "Common/DynamicArray.h"
#include "Common/RefPtr.h"
#include "Page.h"
#include "PhysicalMemory.h"
#include "VirtualAllocator.h"

namespace kernel {

class PhysicalRegion;
class AddressSpace;
class PageFault;

class MemoryManager {
public:
    // TODO: redo most of these constants, they're completely incorrect for x86-64
    static constexpr size_t page_directory_entry_count = 1024;
    static constexpr size_t page_table_entry_count     = 1024;
    static constexpr size_t page_table_address_space   = page_table_entry_count * Page::size;

#ifdef ULTRA_32
    static constexpr Address max_memory_address       = 0xFFFFFFFF;
    static constexpr Address kernel_reserved_base     = 3 * GB;
    static constexpr size_t  kernel_first_table_index = 768;
    static constexpr size_t  kernel_last_table_index  = 1022;
    static constexpr size_t  recursive_entry_index    = 1023;
#elif defined(ULTRA_64)
    static constexpr Address max_memory_address   = 0xFFFFFFFFFFFFFFFF;
    static constexpr Address kernel_reserved_base = max_memory_address - 2 * GB + 1;
    static constexpr Address physical_memory_base = 0xFFFF800000000000;
#endif

    static constexpr size_t  kernel_size              = 3 * MB;
    static constexpr size_t  kernel_heap_initial_size = page_table_address_space;
    static constexpr Address kernel_base_address      = kernel_reserved_base + 1 * MB;
    static constexpr size_t  kernel_pre_reserved_size = kernel_base_address - kernel_reserved_base;
    static constexpr size_t  kernel_reserved_binary   = kernel_pre_reserved_size + kernel_size;
    static constexpr size_t  kernel_reserved_size     = kernel_reserved_binary + kernel_heap_initial_size;
    static constexpr Address kernel_ceiling           = max_memory_address - page_table_address_space + 1;
    static constexpr Address kernel_usable_base       = kernel_reserved_base + kernel_reserved_size;
    static constexpr size_t  kernel_usable_length     = kernel_ceiling - kernel_usable_base;
    static constexpr Address kernel_end_address       = kernel_base_address + kernel_size;
    static constexpr Address kernel_heap_begin        = kernel_end_address;

    static constexpr Address userspace_base           = static_cast<ptr_t>(0x00000000);
    static constexpr size_t  userspace_reserved       = 1 * MB;
    static constexpr Address userspace_usable_base    = userspace_base + userspace_reserved;
    static constexpr Address userspace_usable_ceiling = kernel_reserved_base;
    static constexpr size_t  userspace_usable_length  = userspace_usable_ceiling - userspace_usable_base;

    static void inititalize(const MemoryMap& memory_map);

    static void handle_page_fault(const PageFault& fault);

    static MemoryManager& the();

    void inititalize(AddressSpace& directory);

    [[nodiscard]] RefPtr<Page> allocate_page();
    void                       free_page(Page& page);

#ifdef ULTRA_32
    void set_quickmap_range(const VirtualAllocator::Range& range);

    static constexpr Address physical_to_virtual(Address physical_address)
    {
        ASSERT(physical_address < kernel_reserved_size);

        return physical_address + kernel_reserved_base;
    }

    static constexpr Address virtual_to_physical(Address virtual_address)
    {
        ASSERT(virtual_address > kernel_reserved_base && virtual_address < kernel_end_address);

        return virtual_address - kernel_reserved_base;
    }
#elif defined(ULTRA_64)
    static constexpr Address physical_to_virtual(Address physical_address)
    {
        return physical_memory_base + physical_address;
    }

    static constexpr Address virtual_to_physical(Address virtual_address)
    {
        ASSERT(virtual_address > physical_memory_base);

        if (virtual_address > kernel_reserved_base)
            return virtual_address - kernel_reserved_base;
        else
            return virtual_address - physical_memory_base;
    }
#endif

    const MemoryMap& memory_map() const
    {
        ASSERT(m_memory_map != nullptr);

        return *m_memory_map;
    }

private:
    MemoryManager(const MemoryMap& memory_map);

// Not needed for 64 bit as we have all the phys memory mapped
#ifdef ULTRA_32
    u8*  quickmap_page(const Page&);
    u8*  quickmap_page(Address);
    void unquickmap_page();

    class ScopedPageMapping {
    public:
        ScopedPageMapping(Address physical_address)
        {
            m_pointer = MemoryManager::the().quickmap_page(physical_address);
        }

        ~ScopedPageMapping() { MemoryManager::the().unquickmap_page(); }

        Address raw() { return m_pointer; }

        u8* as_pointer() { return m_pointer; }

    private:
        u8* m_pointer { nullptr };
    };
#endif

private:
    static MemoryManager* s_instance;

    InterruptSafeSpinLock m_lock;

    DynamicArray<PhysicalRegion> m_physical_regions;

    const MemoryMap* m_memory_map;

#ifdef ULTRA_32
    VirtualAllocator::Range m_quickmap_range;
#endif
};
}
