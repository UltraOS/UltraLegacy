#pragma once

#include "Common/DynamicArray.h"
#include "Common/RefPtr.h"
#include "PhysicalMemory.h"
#include "VirtualAllocator.h"

namespace kernel {

    class PhysicalRegion;
    class Page;
    class PageDirectory;
    class PageFault;

    class MemoryManager
    {
    public:
        // 1 page directory entry reserved for recrusive paging
        static constexpr ptr_t  kernel_base          = 0xC0000000;
        static constexpr ptr_t  kernel_ceiling       = 0x100000000ull - (1024 * 4096);
        static constexpr size_t kernel_length        = kernel_ceiling - kernel_base;
        static constexpr ptr_t  kernel_usable_base   = kernel_base + 8 * MB;
        static constexpr size_t kernel_usable_length = kernel_ceiling - kernel_usable_base;

        static constexpr ptr_t  userspace_base           = 0x00000000;
        static constexpr ptr_t  userspace_usable_base    = userspace_base + 1 * MB;
        static constexpr ptr_t  userspace_usable_ceiling = kernel_base;
        static constexpr size_t userspace_usable_length  = userspace_usable_ceiling - userspace_usable_base;

        static void inititalize(const MemoryMap& memory_map);

        static void handle_page_fault(const PageFault& fault);

        static MemoryManager& the();

        void inititalize(PageDirectory& directory);

        [[nodiscard]] RefPtr<Page> allocate_page();
        void free_page(Page& page);

        void set_quickmap_range(const VirtualAllocator::Range& range);
    private:
        MemoryManager(const MemoryMap& memory_map);

        u8* quickmap_page(const Page&);
        u8* quickmap_page(ptr_t);
        void unquickmap_page();

        class ScopedPageMapping
        {
        public:
            ScopedPageMapping(ptr_t physical_address)
            {
                m_pointer = MemoryManager::the().quickmap_page(physical_address);
            }

            ~ScopedPageMapping()
            {
                MemoryManager::the().unquickmap_page();
            }

            ptr_t as_number()
            {
                return reinterpret_cast<ptr_t>(m_pointer);
            }

            u8* as_pointer()
            {
                return m_pointer;
            }

        private:
            u8* m_pointer { nullptr };
        };
    private:
        static MemoryManager* s_instance;
    
        VirtualAllocator::Range      m_quickmap_range;
        DynamicArray<PhysicalRegion> m_physical_regions;
    };
}
