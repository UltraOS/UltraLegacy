#pragma once

#include "Common/DynamicArray.h"
#include "Common/DynamicBitArray.h"
#include "Common/List.h"
#include "Common/RefPtr.h"
#include "Common/UniquePtr.h"
#include "Core/Boot.h"
#include "Core/Registers.h"
#include "MemoryMap.h"
#include "Multitasking/Process.h"
#include "Page.h"
#include "PhysicalRegion.h"
#include "PrivateVirtualRegion.h"
#include "VirtualAllocator.h"
#include "VirtualRegion.h"

namespace kernel {

class AddressSpace;
class PageFault;

class MemoryManager {
    MAKE_SINGLETON(MemoryManager);

public:
#ifdef ULTRA_32
    static constexpr Address max_memory_address = 0xFFFFFFFF;
    static constexpr Address kernel_reserved_base = 3 * GB;
    static constexpr Address kernel_address_space_base = kernel_reserved_base;
    static constexpr size_t kernel_first_table_index = 768;
    static constexpr size_t kernel_last_table_index = 1022;
    static constexpr size_t recursive_entry_index = 1023;
    static constexpr size_t kernel_quickmap_range_size = 512 * KB; // 128 quickmap slots
    static constexpr Address userspace_usable_ceiling = kernel_reserved_base;
#elif defined(ULTRA_64)
    static constexpr Address max_memory_address = 0xFFFFFFFFFFFFFFFF;
    static constexpr Address kernel_reserved_base = max_memory_address - 2 * GB + 1;
    static constexpr Address physical_memory_base = 0xFFFF800000000000;
    static constexpr Address kernel_address_space_base = physical_memory_base;
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

    static bool is_initialized()
    {
        // this is not entirely true as theres a small window where
        // instance is already created but not yet initialized properly
        return s_instance != nullptr;
    }

    void inititalize(AddressSpace& directory);

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

        if (virtual_address >= kernel_reserved_base)
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
        return { m_initial_physical_bytes.load(), m_free_physical_bytes.load() };
    }

    using VR = RefPtr<VirtualRegion>;

    VR allocate_user_stack(StringView purpose, AddressSpace&, size_t length = Process::default_userland_stack_size);
    VR allocate_kernel_stack(StringView purpose, size_t length = Process::default_kernel_stack_size);

    VR allocate_kernel_private(StringView purpose, const Range&);
    VR allocate_kernel_private_anywhere(StringView purpose, size_t length, size_t alignment = Page::size);

    VR allocate_user_private(StringView purpose, const Range&, AddressSpace& = AddressSpace::current());
    VR allocate_user_private_anywhere(StringView purpose, size_t length, size_t alignment = Page::size, AddressSpace& = AddressSpace::current());

    VR allocate_kernel_non_owning(StringView purpose, Range physical_range);
    VR allocate_user_non_owning(StringView purpose, Range physical_range, AddressSpace& = AddressSpace::current());

    String kernel_virtual_regions_debug_dump();

private:
    void allocate_initial_kernel_regions();

    // This API is used by AddressSpace to allocate table pages
    friend class AddressSpace;
    [[nodiscard]] Page allocate_page(bool should_zero = true);
    void free_page(Page& page);

    friend class PrivateVirtualRegion;
    void preallocate(PrivateVirtualRegion&, bool should_zero = true);

    PhysicalRegion* physical_region_responsible_for_page(const Page&);
    VirtualRegion* virtual_region_responsible_for_address(Address);

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
    
    mutable InterruptSafeSpinLock m_virtual_region_lock;

    RedBlackTree<RefPtr<VirtualRegion>, Less<>> m_kernel_virtual_regions;

    // sorted in ascended order therefore can be searched via lower_bound/bianry_search
    DynamicArray<PhysicalRegion> m_physical_regions;

    MemoryMap m_memory_map;

    Atomic<size_t> m_initial_physical_bytes { 0 };
    Atomic<size_t> m_free_physical_bytes { 0 };

#ifdef ULTRA_32
    mutable InterruptSafeSpinLock m_quickmap_lock;
    Range m_quickmap_range;
    DynamicBitArray m_quickmap_slots;
#endif
};
}
