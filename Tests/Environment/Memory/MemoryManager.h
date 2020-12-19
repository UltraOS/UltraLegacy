#pragma once

#include "Common/Types.h"

namespace kernel::MemoryManager {
    static constexpr Address kernel_first_heap_block_base = static_cast<ptr_t>(0);
    static constexpr Address kernel_first_heap_block_size = static_cast<ptr_t>(0);
}