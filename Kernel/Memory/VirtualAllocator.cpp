#include "VirtualAllocator.h"

#include "Core/Runtime.h"

// #define VIRTUAL_ALLOCATOR_DEBUG

namespace kernel {

VirtualAllocator::VirtualAllocator(Address base, size_t length)
    : m_base_range(base, length)
{
}

String VirtualAllocator::debug_dump() const
{
    LockGuard lock_guard(m_lock);

    char number_buffer[21] {};

    String dump;
    dump += "Ranges total: ";

    to_string(m_allocated_ranges.size(), number_buffer, 21);
    dump += number_buffer;
    dump += "\n";

    for (const auto& range : m_allocated_ranges) {
        StackStringBuilder<64> range_str;
        range_str << range;

        dump += range_str.as_view();
        dump += "\n";
    }

    return dump;
}

void VirtualAllocator::reset_with(Address base, size_t length)
{
    LockGuard lock_guard(m_lock);

    m_base_range.reset_with(base, length);
    m_allocated_ranges.clear();
}

bool VirtualAllocator::is_allocated(Address address) const
{
    LockGuard lock_guard(m_lock);

    if (!m_base_range.contains(address))
        return false;

    if (m_allocated_ranges.empty())
        return false;

    auto range = m_allocated_ranges.lower_bound(Range::create_empty_at(address));

    if (range == m_allocated_ranges.end() || range->begin() != address)
        --range;

    return range != m_allocated_ranges.end() && range->contains(address);
}

void VirtualAllocator::merge_ranges_if_possible(RangeIterator range_before, RangeIterator range_after, Range new_range)
{
    if (range_after != m_allocated_ranges.end() && range_after->begin() == new_range.end() && range_before->end() == new_range.begin()) {
        auto merged_range = Range::from_two_pointers(range_before->begin(), range_after->end());
        m_allocated_ranges.remove(range_before);
        m_allocated_ranges.remove(range_after);
        m_allocated_ranges.emplace(merged_range);
    } else if (range_before->end() == new_range.begin()) {
        auto merged_range = Range::from_two_pointers(range_before->begin(), new_range.end());
        m_allocated_ranges.remove(range_before);
        m_allocated_ranges.emplace(merged_range);
    } else if (range_before->begin() == new_range.end()) {
        auto merged_range = Range::from_two_pointers(new_range.begin(), range_before->end());
        m_allocated_ranges.remove(range_before);
        m_allocated_ranges.emplace(merged_range);
    } else {
        m_allocated_ranges.emplace(new_range);
    }
}

VirtualAllocator::Range VirtualAllocator::allocate(size_t length, size_t alignment)
{
    LockGuard lock_guard(m_lock);

    ASSERT(length != 0);

    if (alignment < Page::size)
        alignment = Page::size;

    ASSERT((alignment & ~Page::alignment_mask) == 0);

    auto fail_on_allocation = [](size_t length, size_t alignment) {
        StackStringBuilder error_string;
        error_string << "VirtualAllocator: failed to allocate " << length << " (aligned at " << alignment << ")!";
        runtime::panic(error_string.data());
    };

    length = Page::round_up(length);
    auto aligned_length = length + alignment;

    Range allocated_range;

    if (m_allocated_ranges.empty()) {
        allocated_range = m_base_range;
        bool is_aligned = allocated_range.is_aligned_to(alignment);

        // FIXME: any failure to allocate is considered fatal for now.
        if (is_aligned && m_base_range.length() < length) {
            fail_on_allocation(length, alignment);
        } else if (!is_aligned && m_base_range.length() < aligned_length) {
            fail_on_allocation(length, alignment);
        }

        allocated_range.align_to(alignment);
        allocated_range.set_length(length);
        m_allocated_ranges.emplace(allocated_range);
        return allocated_range;
    }

    auto range_before = m_allocated_ranges.end();
    auto range_after = m_allocated_ranges.end();

    auto gap_begin = m_base_range.begin();

    for (range_after = m_allocated_ranges.begin(); range_after != m_allocated_ranges.end(); ++range_after) {
        auto potential_range = Range::from_two_pointers(gap_begin, range_after->begin());
        bool is_aligned = potential_range.is_aligned_to(alignment);

        if ((is_aligned && potential_range.length() < length) || (!is_aligned && potential_range.length() < aligned_length)) {
            range_before = range_after;
            gap_begin = range_after->end();
            continue;
        }

        allocated_range = potential_range;
        break;
    }

    if (allocated_range.empty()) {
        auto potential_range = Range::from_two_pointers(gap_begin, m_base_range.end());
        bool is_aligned = potential_range.is_aligned_to(alignment);

        if ((is_aligned && potential_range.length() < length) || (!is_aligned && potential_range.length() < aligned_length)) {
            fail_on_allocation(length, alignment);
        }

        range_before = --m_allocated_ranges.end();
        allocated_range = potential_range;
        allocated_range.set_length(length);
        allocated_range.align_to(alignment);
    }

    allocated_range.align_to(alignment);

    merge_ranges_if_possible(range_before, range_after, allocated_range);

#ifdef VIRTUAL_ALLOCATOR_DEBUG
    log() << "VirtualAllocator: allocating a new range " << allocated_range;
#endif

    return allocated_range;
}

VirtualAllocator::Range VirtualAllocator::allocate(Range range)
{
#ifdef VIRTUAL_ALLOCATOR_DEBUG
    log() << "VirtualAllocator: trying to allocate range " << range;
#endif

    LockGuard lock_guard(m_lock);

    ASSERT(range.empty() == false);
    ASSERT(contains(range));

    auto fail_on_allocated_range = [](Range range) {
        StackStringBuilder error_string;
        error_string << "VirtualAllocator: Couldn't allocate range ( " << range << " )";
        runtime::panic(error_string.data());
    };

    auto begin = Page::round_down(range.begin());
    auto length = Page::round_up(range.length() + range.begin() - begin);

    range = { begin, length };

    if (m_allocated_ranges.empty()) {
        m_allocated_ranges.emplace(range);
        return range;
    }

    auto it = m_allocated_ranges.lower_bound(range);

    if (it == m_allocated_ranges.end() || it->begin() != range.begin())
        --it;

    // TODO: implement advance(iterator, by)?
    auto range_after = it;
    ++range_after;

    if (range_after != m_allocated_ranges.end() && range_after->contains(range.end() - 1))
        fail_on_allocated_range(range);

    if (it->begin() == range.begin() || it->contains(range.begin()))
        fail_on_allocated_range(range);

    merge_ranges_if_possible(it, range_after, range);

    return range;
}

bool VirtualAllocator::contains(Address address)
{
    return m_base_range.contains(address);
}

bool VirtualAllocator::contains(const Range& range)
{
    return m_base_range.contains(range.begin()) && m_base_range.contains(range.end() - 1);
}

void VirtualAllocator::deallocate(const Range& range)
{
#ifdef VIRTUAL_ALLOCATOR_DEBUG
    log() << "VirtualAllocator: deallocating range " << range;
#endif

    LockGuard lock_guard(m_lock);

    if (!contains(range)) {
        StackStringBuilder error_string;
        error_string << "VirtualAllocator: range " << range << " doesn't belong to this allocator!";
        runtime::panic(error_string.data());
    }

    auto fail_on_unknown_range = [](const Range& range) {
        StackStringBuilder error_string;
        error_string << "VirtualAllocator: range " << range << " wasn't found as allocated!";
        runtime::panic(error_string.data());
    };

    if (m_allocated_ranges.empty())
        fail_on_unknown_range(range);

    auto it = m_allocated_ranges.lower_bound(range);

    if (it == m_allocated_ranges.end() || it->begin() != range.begin())
        --it;

    if (!it->contains(range))
        fail_on_unknown_range(range);

    auto range_before = it->clipped_with_end(range.begin());
    auto range_after = it->clipped_with_begin(range.end());

    m_allocated_ranges.remove(it);

    if (range_before)
        m_allocated_ranges.emplace(range_before);
    if (range_after)
        m_allocated_ranges.emplace(range_after);
}

}
