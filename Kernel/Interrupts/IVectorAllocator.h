#pragma once

#include "Common/DynamicBitArray.h"
#include "Core/Runtime.h"
#include "Memory/Range.h"

namespace kernel {

using VectorRange = BasicRange<u16>;
inline static constexpr u16 any_vector = 0xFFFF;

class InterruptSafeSpinLock;

class IVectorAllocator {
    MAKE_SINGLETON(IVectorAllocator);

public:
    static void initialize();

    static IVectorAllocator& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void allocate_vector(u16);
    u16 allocate_vector();
    void allocate_range(VectorRange);

    void free_vector(u16);
    void free_range(VectorRange);

private:
    InterruptSafeSpinLock* m_lock { nullptr };
    DynamicBitArray m_allocation_map;

    static IVectorAllocator* s_instance;
};

}
