#include "VirtualAllocator.h"

#include "Core/Runtime.h"

// #define VIRTUAL_ALLOCATOR_DEBUG

namespace kernel {

VirtualAllocator::VirtualAllocator(Address begin, Address end)
    : m_base_range(begin, end - begin)
{
}

String VirtualAllocator::debug_dump() const
{
    LockGuard lock_guard(m_lock);

    String dump;
    dump << "Base range: " << m_base_range.begin() << " -> " << m_base_range.end() << "\n";
    dump << "Ranges total: " << m_allocated_ranges.size() << "\n";

    for (const auto& range : m_allocated_ranges) {
        dump << range << "\n";
    }

    return dump;
}

void VirtualAllocator::reset_with(Address begin, Address end)
{
    LockGuard lock_guard(m_lock);

    m_base_range.reset_with_two_pointers(begin, end);
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

    if (range == m_allocated_ranges.end() || range->begin() != address) {
        if (range == m_allocated_ranges.begin())
            return false;

        --range;
    }

    return range->contains(address);
}

void VirtualAllocator::merge_and_emplace(RangeIterator before, RangeIterator after, Range new_range)
{
    bool before_mergeable = before != m_allocated_ranges.end() && before->end() == new_range.begin();
    bool after_mergeable = after != m_allocated_ranges.end() && after->begin() == new_range.end();

    if (before_mergeable && after_mergeable) {
        auto merged_range = Range::from_two_pointers(before->begin(), after->end());
        m_allocated_ranges.remove(before);
        m_allocated_ranges.remove(after);
        m_allocated_ranges.emplace(merged_range);
    } else if (before_mergeable) {
        auto merged_range = Range::from_two_pointers(before->begin(), new_range.end());
        m_allocated_ranges.remove(before);
        m_allocated_ranges.emplace(merged_range);
    } else if (after_mergeable) {
        auto merged_range = Range::from_two_pointers(new_range.begin(), after->end());
        m_allocated_ranges.remove(after);
        m_allocated_ranges.emplace(merged_range);
    } else { // entirely new range, cannot merge anything :(
        m_allocated_ranges.emplace(new_range);
    }
}

Range VirtualAllocator::allocate(size_t length, size_t alignment)
{
    LockGuard lock_guard(m_lock);

    ASSERT(length != 0);

    if (alignment < Page::size)
        alignment = Page::size;

    ASSERT((alignment & ~Page::alignment_mask) == 0);

    auto fail_on_allocation = [](size_t length, size_t alignment) {
        String error_string;
        error_string << "VirtualAllocator: failed to allocate " << length << " (aligned at " << alignment << ")!";
        runtime::panic(error_string.data());
    };

    auto rounded_up_length = Page::round_up(length);

    if (rounded_up_length < length) {
        String error_string;
        error_string << "VirtualAllocator: Length overflow with length=" << length << ", alignment=" << alignment;
        runtime::panic(error_string.data());
    }

#ifdef VIRTUAL_ALLOCATOR_DEBUG
    log() << "VirtualAllocator: trying to allocate length=" << rounded_up_length << ", alignment=" << alignment;
#endif

    length = rounded_up_length;

    Range allocated_range;

    if (m_allocated_ranges.empty()) {
        allocated_range = m_base_range.aligned_to(alignment);

        // FIXME: any failure to allocate is considered fatal for now.
        if (!allocated_range || allocated_range.length() < length)
            fail_on_allocation(length, alignment);

        allocated_range.set_length(length);
        m_allocated_ranges.emplace(allocated_range);
        return allocated_range;
    }

    auto range_before = m_allocated_ranges.end();
    auto range_after = m_allocated_ranges.end();

    auto gap_begin = m_base_range.begin();

    for (range_after = m_allocated_ranges.begin(); range_after != m_allocated_ranges.end(); ++range_after) {
        auto potential_range = Range::from_two_pointers(gap_begin, range_after->begin()).aligned_to(alignment);

        if (!potential_range || potential_range.length() < length) {
            range_before = range_after;
            gap_begin = range_after->end();
            continue;
        }

        allocated_range = potential_range;
        allocated_range.set_length(length);
        break;
    }

    if (allocated_range.empty()) {
        auto potential_range = Range::from_two_pointers(gap_begin, m_base_range.end()).aligned_to(alignment);

        if (!potential_range || potential_range.length() < length)
            fail_on_allocation(length, alignment);

        range_before = --m_allocated_ranges.end();
        allocated_range = potential_range;
        allocated_range.set_length(length);
    }

    merge_and_emplace(range_before, range_after, allocated_range);

#ifdef VIRTUAL_ALLOCATOR_DEBUG
    log() << "VirtualAllocator: allocating a new range " << allocated_range;
#endif

    return allocated_range;
}

Range VirtualAllocator::allocate(Range range)
{
#ifdef VIRTUAL_ALLOCATOR_DEBUG
    log() << "VirtualAllocator: trying to allocate range " << range;
#endif

    LockGuard lock_guard(m_lock);

    ASSERT(range.empty() == false);
    ASSERT(contains(range));

    auto fail_on_allocated_range = [](Range range) {
        String error_string;
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
    auto range_after = it;

    if (it == m_allocated_ranges.end() || it->begin() != range.begin()) {
        if (it != m_allocated_ranges.begin()) {
            --it;
        } else {
            range_after = it;
            it = m_allocated_ranges.end();
        }
    } else {
        ++range_after;
    }

    if (range_after != m_allocated_ranges.end() && range_after->contains(range.end() - 1))
        fail_on_allocated_range(range);

    if (it != m_allocated_ranges.end() && (it->begin() == range.begin() || it->contains(range.begin())))
        fail_on_allocated_range(range);

    merge_and_emplace(it, range_after, range);

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
        String error_string;
        error_string << "VirtualAllocator: range " << range << " doesn't belong to this allocator!";
        runtime::panic(error_string.data());
    }

    auto fail_on_unknown_range = [](const Range& range) {
        String error_string;
        error_string << "VirtualAllocator: range " << range << " wasn't found as allocated!";
        runtime::panic(error_string.data());
    };

    if (m_allocated_ranges.empty())
        fail_on_unknown_range(range);

    auto it = m_allocated_ranges.lower_bound(range);

    if (it == m_allocated_ranges.end() || it->begin() != range.begin()) {
        if (it == m_allocated_ranges.begin())
            fail_on_unknown_range(range);
        else
            --it;
    }

    if (it == m_allocated_ranges.end() || !it->contains(range))
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
