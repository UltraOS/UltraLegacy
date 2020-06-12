#pragma once

#include "PhysicalMemory.h"

namespace kernel {

    class MemoryManager
    {
    public:
        static constexpr size_t page_size = 4096;

        static void inititalize(const MemoryMap& memory_map);

        static MemoryManager& the();
    private:
        MemoryManager(const MemoryMap& memory_map);

        static MemoryManager* s_instance;
    };
}
