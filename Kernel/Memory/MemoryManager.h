#pragma once

#include "PhysicalMemory.h"
#include "Common/DynamicArray.h"
#include "Common/RefPtr.h"

namespace kernel {

    class PhysicalRegion;
    class Page;

    class MemoryManager
    {
    public:
        static void inititalize(const MemoryMap& memory_map);

        static MemoryManager& the();

        [[nodiscard]] RefPtr<Page> allocate_page();
        void free_page(Page& page);
    private:
        MemoryManager(const MemoryMap& memory_map);

    private:
        static MemoryManager* s_instance;
    
        DynamicArray<PhysicalRegion> m_physical_regions;
    };
}
