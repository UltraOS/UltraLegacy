#pragma once

#include "Common/DynamicBitArray.h"
#include "Common/Lock.h"
#include "Common/Logger.h"
#include "Common/Optional.h"
#include "Common/Types.h"
#include "Common/UniquePtr.h"
#include "Range.h"

namespace kernel {

class Page;

class PhysicalRegion {
public:
    explicit PhysicalRegion(const Range& range);

    const Range& range() const { return m_range; }
    Address begin() const { return m_range.begin(); }
    Address end() const { return m_range.end(); }

    size_t free_page_count() const { return m_free_pages.load(MemoryOrder::ACQUIRE); }
    bool has_free_pages() const { return m_free_pages.load(MemoryOrder::ACQUIRE); }

    [[nodiscard]] Optional<DynamicArray<Page>> allocate_pages(size_t count);
    [[nodiscard]] Optional<Page> allocate_page();
    void free_page(const Page& page);

    template <typename LoggerT>
    friend LoggerT& operator<<(LoggerT&& logger, const PhysicalRegion& region)
    {
        logger << "Range: " << region.m_range << " pages:" << region.free_page_count();

        return logger;
    }

    friend bool operator<(Address address, const PhysicalRegion& region)
    {
        return address < region.begin();
    }

    friend bool operator<(const PhysicalRegion& region, Address address)
    {
        return region.begin() < address;
    }

    friend bool operator<(Address address, const UniquePtr<PhysicalRegion>& region)
    {
        ASSERT(region);
        return address < region->begin();
    }

    friend bool operator<(const UniquePtr<PhysicalRegion>& region, Address address)
    {
        ASSERT(region);
        return region->begin() < address;
    }

private:
    Address bit_as_physical_address(size_t bit);
    size_t physical_address_as_bit(Address address);

private:
    InterruptSafeSpinLock m_lock;
    Range m_range;
    Atomic<size_t> m_free_pages { 0 };
    size_t m_next_hint { 0 };
    DynamicBitArray m_allocation_map;
};
}
