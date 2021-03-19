#include "Utilities.h"

#include "AddressSpace.h"
#include "MemoryManager.h"

namespace kernel {

DynamicArray<Range> accumulate_contiguous_physical_ranges(Address virtual_address, size_t byte_length, size_t max_bytes_per_range)
{
    ASSERT(max_bytes_per_range >= Page::size);
    ASSERT_PAGE_ALIGNED(max_bytes_per_range);
    ASSERT(virtual_address > MemoryManager::kernel_address_space_base);
    auto& address_space = AddressSpace::current();

    DynamicArray<Range> contiguous_ranges;

    Address current_address = virtual_address;
    Range current_range {};

    while (byte_length) {
        byte_length -= min(Page::size, byte_length);

        auto physical_address = address_space.physical_address_of(current_address);
        ASSERT(physical_address);

        if (current_range) {
            if (current_range.end() == physical_address) {
                current_range.extend_by(Page::size);
            } else {
                contiguous_ranges.append(current_range);
                current_range = { physical_address, Page::size };
            }
        } else {
            current_range = { physical_address, Page::size };
        }

        if (current_range.length() == max_bytes_per_range) {
            contiguous_ranges.append(current_range);
            current_range.reset();
        }

        current_address += Page::size;
    }

    if (current_range)
        contiguous_ranges.append(current_range);

    return contiguous_ranges;
}

}