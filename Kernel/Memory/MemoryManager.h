#pragma once

#include "Common/DynamicArray.h"
#include "Common/DynamicBitArray.h"
#include "Common/RefPtr.h"
#include "Core/Boot.h"
#include "Core/Registers.h"
#include "MemoryMap.h"
#include "Page.h"
#include "VirtualAllocator.h"

namespace kernel {

class PhysicalRegion;
class AddressSpace;
class PageFault;

class MemoryManager {
    MAKE_SINGLETON(MemoryManager);

public:
#ifdef ULTRA_32
    static constexpr Address max_memory_address = 0xFFFFFFFF;
    static constexpr Address kernel_reserved_base = 3 * GB;
    static constexpr size_t kernel_first_table_index = 768;
    static constexpr size_t kernel_last_table_index = 1022;
    static constexpr size_t recursive_entry_index = 1023;
    static constexpr size_t kernel_quickmap_range_size = 512 * KB; // 128 quickmap slots
    static constexpr Address userspace_usable_ceiling = kernel_reserved_base;
#elif defined(ULTRA_64)
    static constexpr Address max_memory_address = 0xFFFFFFFFFFFFFFFF;
    static constexpr Address kernel_reserved_base = max_memory_address - 2 * GB + 1;
    static constexpr Address physical_memory_base = 0xFFFF800000000000;
    static constexpr Address userspace_usable_ceiling = 0x0000800000000000;
    static constexpr size_t kernel_first_table_index = 256;
    static constexpr size_t kernel_last_table_index = 511;
#endif
    static constexpr size_t kernel_image_size = 3 * MB;
    static constexpr size_t kernel_first_heap_block_size = 4 * MB;

    static constexpr Address kernel_image_base = kernel_reserved_base + 1 * MB;
    static constexpr Address kernel_reserved_ceiling = kernel_image_base + kernel_image_size;
    static constexpr Address kernel_first_heap_block_base = kernel_reserved_ceiling;
    static constexpr Address kernel_first_heap_block_ceiling = kernel_first_heap_block_base + kernel_first_heap_block_size;

#ifdef ULTRA_32
    static constexpr size_t kernel_quickmap_range_base = kernel_first_heap_block_ceiling;
#endif

    static constexpr size_t kernel_reserved_size = kernel_reserved_ceiling - kernel_reserved_base;

    static Range kernel_reserved_range()
    {
        return Range::from_two_pointers(
            kernel_reserved_base,
            kernel_image_base + kernel_image_size + kernel_first_heap_block_size
#ifdef ULTRA_32
                + kernel_quickmap_range_size
#endif
        );
    }

    Address kernel_address_space_free_base() const
    {
#ifdef ULTRA_32
        return kernel_reserved_range().end();
#elif defined(ULTRA_64)
        // 64 bit kernel's free base depends on the highest physical address in the memory map,
        // because we have to have direct access to all physical memory via 'physical_memory_base + physaddr'.
        // Since we always direct map the first 4 physical gigabytes we pick the maximum of the two
        auto rounded_up_physical = Page::round_up(m_memory_map.highest_address());
        return max(physical_to_virtual(rounded_up_physical), physical_to_virtual(4 * GB));
#endif
    }

    static Address kernel_address_space_free_ceiling();

    static constexpr Address userspace_base = static_cast<ptr_t>(0x00000000);
    static constexpr size_t userspace_reserved = 1 * MB;
    static constexpr Address userspace_usable_base = userspace_base + userspace_reserved;
    static constexpr size_t userspace_usable_length = userspace_usable_ceiling - userspace_usable_base;

    // Reserves kernel image from the memory map & initializes the heap
    static void early_initialize(LoaderContext*);

    static void inititalize();

    static void handle_page_fault(const RegisterState&, const PageFault&);

    static MemoryManager& the();

    void inititalize(AddressSpace& directory);

    [[nodiscard]] RefPtr<Page> allocate_page(bool should_zero = true);
    void free_page(Page& page);

#ifdef ULTRA_32
    void set_quickmap_range(const Range& range);

    static constexpr Address physical_to_virtual(Address physical_address)
    {
        ASSERT(physical_address < kernel_reserved_size);

        return physical_address + kernel_reserved_base;
    }

    static constexpr Address virtual_to_physical(Address virtual_address)
    {
        ASSERT(virtual_address >= kernel_reserved_base && virtual_address < kernel_reserved_ceiling);

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
        return m_memory_map;
    }

    static const LoaderContext* loader_context()
    {
        return s_loader_context;
    }

    struct PhysicalStats {
        size_t total_bytes;
        size_t free_bytes;
    };

    [[nodiscard]] PhysicalStats physical_stats() const
    {
        LockGuard lock_guard(m_lock);
        return { m_initial_physical_bytes, m_free_physical_bytes };
    }

    void force_preallocate(const Range& range, bool should_zero = false);

private:
// Not needed for 64 bit as we have all the phys memory mapped
#ifdef ULTRA_32
    u8* quickmap_page(const Page&);
    u8* quickmap_page(Address);
    void unquickmap_page(Address);

    class ScopedPageMapping {
    public:
        explicit ScopedPageMapping(Address physical_address)
        {
            m_pointer = MemoryManager::the().quickmap_page(physical_address);
        }

        ~ScopedPageMapping() { MemoryManager::the().unquickmap_page(raw()); }

        Address raw() { return m_pointer; }

        u8* as_pointer() { return m_pointer; }

    private:
        u8* m_pointer { nullptr };
    };
#endif

private:
    static MemoryManager* s_instance;
    static LoaderContext* s_loader_context;

    mutable RecursiveInterruptSafeSpinLock m_lock;

    DynamicArray<PhysicalRegion> m_physical_regions;

    MemoryMap m_memory_map;

    size_t m_initial_physical_bytes { 0 };
    size_t m_free_physical_bytes { 0 };

#ifdef ULTRA_32
    InterruptSafeSpinLock m_quickmap_lock;
    Range m_quickmap_range;
    DynamicBitArray m_quickmap_slots;
#endif
};
}
