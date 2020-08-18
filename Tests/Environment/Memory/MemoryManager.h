#pragma once

#include "Common/Types.h"

namespace kernel::MemoryManager {
    static constexpr Address kernel_heap_begin        = static_cast<ptr_t>(0);
    static constexpr Address kernel_heap_initial_size = static_cast<ptr_t>(0);
}