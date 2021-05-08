#include "BootAllocator.h"
#include "Page.h"

namespace kernel {

alignas(BootAllocator) static u8 instance_storage[sizeof(BootAllocator)];

BootAllocator::BootAllocator(MemoryMap memory_map)
    : m_memory_map(memory_map)
{
}

void BootAllocator::initialize(MemoryMap memory_map)
{
    static bool is_ctored = false;
    ASSERT(is_ctored == false);

    new (instance_storage) BootAllocator(memory_map);
    is_ctored = true;
}

BootAllocator& BootAllocator::the()
{
    return *reinterpret_cast<BootAllocator*>(instance_storage);
}

Address64 BootAllocator::reserve_contiguous(size_t page_count, Address64 lower_limit, Address64 upper_limit, Tag tag)
{
    ASSERT(m_did_release == false);

    auto fail_on_allocation = [&]() {
        StackString error_string;
        error_string << "BootAllocator: failed to allocate "
                     << page_count << " pages with tag " << tag_to_string(tag)
                     << " within " << lower_limit << "-" << upper_limit
                     << " reason: "
                     << "couldn't find a suitable range";
        runtime::panic(error_string.data());
    };

    auto bytes_to_allocate = page_count * Page::size;

    // invaid input
    if (lower_limit >= upper_limit)
        fail_on_allocation();

    // search gap is too small
    if (lower_limit + bytes_to_allocate > upper_limit)
        fail_on_allocation();

    // overflow
    if (lower_limit + bytes_to_allocate < lower_limit)
        fail_on_allocation();

    auto should_look_further = [&](const MemoryMap::PhysicalRange& current_range) -> bool {
        if (current_range.end() >= upper_limit)
            return false;

        auto bytes_left_in_gap = upper_limit - current_range.end();

        if (bytes_left_in_gap < bytes_to_allocate)
            return false;

        return true;
    };

    auto picked_range = lower_bound(m_memory_map.begin(), m_memory_map.end(), lower_limit);

    if (picked_range == m_memory_map.end() || picked_range->begin() != lower_limit) {
        if (picked_range == m_memory_map.begin())
            fail_on_allocation();

        --picked_range;
    }

    for (; picked_range != m_memory_map.end(); ++picked_range) {
        bool is_bad_range = false;

        if (picked_range->type != MemoryMap::PhysicalRange::Type::FREE)
            is_bad_range = true;

        if (picked_range->length() < bytes_to_allocate)
            is_bad_range = true;

        if (picked_range->begin() + bytes_to_allocate > upper_limit)
            is_bad_range = true;

        if (is_bad_range) {
            if (should_look_further(*picked_range))
                continue;

            fail_on_allocation();
        }

        break;
    }

    MemoryMap::PhysicalRange allocated_range(
        max(lower_limit, picked_range->begin()),
        bytes_to_allocate,
        native_type_from_tag(tag));

    auto new_ranges = picked_range->shatter_against(allocated_range);

    auto index_of_range = picked_range - m_memory_map.begin();
    auto current_index = index_of_range;

    for (auto& new_range : new_ranges) {
        bool range_is_valid = new_range && (new_range.length() >= Page::size || new_range.is_reserved());

        if (!range_is_valid)
            continue;

        if (current_index == index_of_range) {
            m_memory_map.at(current_index++) = new_range;
        } else {
            m_memory_map.emplace_range_at(current_index++, new_range);
        }
    }

    // we might have some new trivially mergeable ranges after shatter, let's correct them
    m_memory_map.correct_overlapping_ranges(index_of_range ? index_of_range - 1 : index_of_range);

    return allocated_range.begin();
}

Address64 BootAllocator::reserve_at(Address64 address, size_t page_count, Tag tag)
{
    return reserve_contiguous(page_count, address, address + page_count * Page::size, tag);
}

// Using any of the reserve() functions is not allowed after this call
MemoryMap BootAllocator::release()
{
    m_did_release = true;
    return m_memory_map;
}

}
