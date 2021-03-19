#pragma once

#include "Common/DynamicArray.h"
#include "Range.h"

namespace kernel {

DynamicArray<Range> accumulate_contiguous_physical_ranges(Address virtual_address, size_t byte_length, size_t max_bytes_per_range);

}