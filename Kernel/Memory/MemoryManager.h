#pragma once

#include "Common/DynamicArray.h"
#include "Common/RefPtr.h"
#include "PhysicalMemory.h"
#include "Page.h"
#include "VirtualAllocator.h"

namespace kernel {

    class PhysicalRegion;
    class PageDirectory;
    class PageFault;

    class MemoryManager
    {
    public:
        static constexpr size_t page_directory_entry_count = 1024;
        static constexpr size_t page_table_entry_count     = 1024;
        static constexpr size_t page_table_address_space   = page_table_entry_count * Page::size;;
        static constexpr ptr_t  max_memory_address         = 0xFFFFFFFF;

        static constexpr size_t kernel_size              = 3 * MB;
        static constexpr size_t kernel_heap_initial_size = page_table_address_space;
        static constexpr ptr_t  kernel_reserved_base     = 3 * GB;
        static constexpr ptr_t  kernel_base_address      = kernel_reserved_base + 1 * MB;
        static constexpr size_t kernel_pre_reserved_size = kernel_base_address - kernel_reserved_base;
        static constexpr size_t kernel_reserved_size     = kernel_pre_reserved_size + kernel_size + kernel_heap_initial_size;
        static constexpr ptr_t  kernel_ceiling           = max_memory_address - page_table_address_space + 1;
        static constexpr ptr_t  kernel_usable_base       = kernel_reserved_base + kernel_reserved_size;
        static constexpr size_t kernel_usable_length     = kernel_ceiling - kernel_usable_base;
        static constexpr ptr_t  kernel_end_address       = kernel_base_address + kernel_size;
        static constexpr ptr_t  kernel_heap_begin        = kernel_end_address;
        static constexpr size_t kernel_first_table_index = 768;
        static constexpr size_t kernel_last_table_index  = 1022;
        static constexpr size_t recursive_entry_index    = 1023;

        static constexpr ptr_t  userspace_base           = 0x00000000;
        static constexpr size_t userspace_reserved       = 1 * MB;
        static constexpr ptr_t  userspace_usable_base    = userspace_base + userspace_reserved;
        static constexpr ptr_t  userspace_usable_ceiling = kernel_reserved_base;
        static constexpr size_t userspace_usable_length  = userspace_usable_ceiling - userspace_usable_base;

        static void inititalize(const MemoryMap& memory_map);

        static void handle_page_fault(const PageFault& fault);

        static MemoryManager& the();

        void inititalize(PageDirectory& directory);

        [[nodiscard]] RefPtr<Page> allocate_page();
        void free_page(Page& page);

        void set_quickmap_range(const VirtualAllocator::Range& range);

        static constexpr ptr_t kernel_address_as_physical(ptr_t virtual_address)
        {
            ASSERT(virtual_address > kernel_reserved_base &&
                   virtual_address < kernel_end_address);

            return virtual_address - kernel_reserved_base;
        }

        static constexpr ptr_t physical_address_as_kernel(ptr_t physical_address)
        {
            ASSERT(physical_address < kernel_reserved_size);

            return physical_address + kernel_reserved_base;
        }

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
