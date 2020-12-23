#pragma once

#include "Common/DynamicBitArray.h"
#include "Common/Logger.h"
#include "Common/RefPtr.h"
#include "Common/Types.h"

namespace kernel {

class Page;

class PhysicalRegion {
public:
    PhysicalRegion(Address starting_address, size_t length);

    Address starting_address() const { return m_starting_address; }
    size_t free_page_count() const { return m_free_pages; }
    bool has_free_pages() const { return m_free_pages; }

    [[nodiscard]] RefPtr<Page> allocate_page();
    void free_page(const Page& page);

    bool contains(const Page& page);

    template <typename LoggerT>
    friend LoggerT& operator<<(LoggerT&& logger, const PhysicalRegion& region)
    {
        logger << "Starting address:" << region.m_starting_address << " pages:" << region.free_page_count();

        return logger;
    }

private:
    Address bit_as_physical_address(size_t bit);
    size_t physical_address_as_bit(Address address);

private:
    Address m_starting_address { nullptr };
    size_t m_free_pages { 0 };
    size_t m_next_hint { 0 };
    DynamicBitArray m_allocation_map;
};
}
