#pragma once

#include "PhysicalMemory.h"
#include "Common/DynamicArray.h"

namespace kernel {

    class PhysicalRegion;

    class MemoryManager
    {
    public:
        static void inititalize(const MemoryMap& memory_map);

        static MemoryManager& the();
    private:
        MemoryManager(const MemoryMap& memory_map);

    private:
        static MemoryManager* s_instance;
    
        DynamicArray<PhysicalRegion> m_physical_regions;
    };
}
