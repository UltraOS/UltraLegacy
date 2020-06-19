#pragma once

#include "PhysicalMemory.h"
#include "Common/DynamicArray.h"
#include "Common/RefPtr.h"

namespace kernel {

    class PhysicalRegion;
    class Page;
    class PageDirectory;

    class MemoryManager
    {
    public:
        // 1 page directory entry reserved for recrusive paging
        static constexpr ptr_t  kernel_base    = 0xC0000000;
        static constexpr ptr_t  kernel_ceiling = 0x100000000ull - (1024 * 4096);
        static constexpr size_t kernel_length  = kernel_ceiling - kernel_base;
        static constexpr ptr_t  kernel_usable_virtual_base = kernel_base + 8 * MB;
        static constexpr size_t kernel_usable_virtual_length = kernel_ceiling - kernel_usable_virtual_base;

        static constexpr ptr_t  userspace_base    = 0x00000000;
        static constexpr ptr_t  userspace_ceiling = kernel_base;
        static constexpr size_t userspace_length  = userspace_ceiling - userspace_base;

        static void inititalize(const MemoryMap& memory_map);

        static MemoryManager& the();

        void inititalize(PageDirectory& directory);

        [[nodiscard]] RefPtr<Page> allocate_page();
        void free_page(Page& page);
    private:
        MemoryManager(const MemoryMap& memory_map);

    private:
        static MemoryManager* s_instance;
    
        DynamicArray<PhysicalRegion> m_physical_regions;
    };
}
