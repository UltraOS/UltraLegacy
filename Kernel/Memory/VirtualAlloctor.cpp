#include "VirtualAllocator.h"

// #define VIRTUAL_ALLOCATOR_DEBUG

namespace kernel {

VirtualAllocator::Range::Range(Address start, Address length) : m_start(start), m_length(length) { }

Address VirtualAllocator::Range::begin() const
{
    return m_start;
}

Address VirtualAllocator::Range::end() const
{
    return m_start + m_length;
}

size_t VirtualAllocator::Range::length() const
{
    return m_length;
}

bool VirtualAllocator::Range::contains(Address address)
{
    return address >= begin() && address < end();
}

void VirtualAllocator::Range::set(Address start, Address length)
{
    m_start  = start;
    m_length = length;
}

bool VirtualAllocator::Range::operator==(const Range& other) const
{
    return m_start == other.m_start && m_length == other.m_length;
}

VirtualAllocator::VirtualAllocator(Address base, size_t length) : m_full_range(base, length)
{
    m_free_ranges.emplace(m_full_range);
}

void VirtualAllocator::set_range(Address base, size_t length)
{
    m_full_range.set(base, length);
    m_allocated_ranges.clear();
    m_free_ranges.clear();

    m_free_ranges.emplace(m_full_range);
}

bool VirtualAllocator::is_allocated(Address address)
{
    // use binary search here

    for (auto& range: m_allocated_ranges) {
        if (range.contains(address))
            return true;
    }

    return false;
}

VirtualAllocator::Range VirtualAllocator::allocate_range(size_t length)
{
    if (length == 0) {
        warning() << "VirtualAllocator: looks like somebody tried to allocate a 0 length range (could be a bug?)";
        return {};
    }

    size_t range_index = 0;

    for (range_index = 0; range_index < m_free_ranges.size(); ++range_index) {
        if (m_free_ranges[range_index].length() >= length)
            break;
    }

    if (range_index == m_free_ranges.size()) {
        error() << "VirtualAlloctor: out of ranges!";
        hang();
    }

    // allocate
    auto& initial_range   = m_free_ranges[range_index];
    auto& allocated_range = m_allocated_ranges.emplace(initial_range.begin(), length);

#ifdef VIRTUAL_ALLOCATOR_DEBUG
    log() << "VirtualAllocator: allocating a new range " << allocated_range;
#endif

    if (initial_range == allocated_range)
        m_free_ranges.erase_at(range_index);
    else
        initial_range.set(allocated_range.end(), initial_range.length() - length);

    return allocated_range;
}

bool VirtualAllocator::contains(Address address)
{
    return m_full_range.contains(address);
}

bool VirtualAllocator::contains(const Range& range)
{
    return m_full_range.contains(range.begin()) && m_full_range.contains(range.end() - 1);
}

void VirtualAllocator::deallocate_range(const Range& range)
{
    if (!contains(range)) {
        error() << "VirtualAlloctor: range " << range << " doesn't belong to this allocator!";
        hang();
    }

    for (size_t range_index = 0; range_index < m_free_ranges.size(); ++range_index) {
        auto& allocated_range = m_allocated_ranges[range_index];

        if (allocated_range == range) {
#ifdef VIRTUAL_ALLOCATOR_DEBUG
            info() << "VirtualAllocator: deallocating range " << allocated_range;
#endif
            return_back_to_free_pool(range_index);
            return;
        }
    }

    error() << "VirtualAlloctor: range " << range << " wasn't found as allocated!";
    hang();
}

void VirtualAllocator::deallocate_range(Address base_address)
{
    if (!contains(base_address)) {
        error() << "VirtualAlloctor: address " << base_address << " doesn't belong to this allocator!";
        hang();
    }

    for (size_t range_index = 0; range_index < m_free_ranges.size(); ++range_index) {
        auto& allocated_range = m_allocated_ranges[range_index];

        if (allocated_range.begin() == base_address) {
#ifdef VIRTUAL_ALLOCATOR_DEBUG
            info() << "VirtualAllocator: deallocating range " << allocated_range;
#endif
            return_back_to_free_pool(range_index);
            return;
        }
    }

    error() << "VirtualAlloctor: address " << base_address << " wasn't found as allocated!";
    hang();
}

void VirtualAllocator::return_back_to_free_pool(size_t allocated_index)
{
    m_free_ranges.emplace(m_allocated_ranges[allocated_index]);
    m_allocated_ranges.erase_at(allocated_index);
}
}
