#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/Runtime.h"

namespace kernel {

class Page {
public:
#ifdef ULTRA_32
    static constexpr size_t huge_size = 4 * MB;
    static constexpr Address alignment_mask = 0xFFFFF000;
#elif defined(ULTRA_64)
    static constexpr size_t huge_size = 2 * MB;
    static constexpr Address alignment_mask = 0xFFFFFFFFFFFFF000;
#endif

    static constexpr size_t size = 4096;

    explicit Page(Address physical_address);

    Address address() const;

    static constexpr bool is_aligned(Address address) { return (address % size) == 0; }

    static constexpr size_t round_up(size_t size)
    {
        if (size == 0)
            return Page::size;

        return size & ~Page::alignment_mask ? (size + Page::size) & Page::alignment_mask : size;
    }

    static constexpr size_t round_down(size_t size)
    {
        return size & ~Page::alignment_mask ? size & Page::alignment_mask : size;
    }

    static constexpr bool is_huge_aligned(Address address)
    {
        return (address % huge_size) == 0;
    }

    ~Page();

private:
    Address m_physical_address;
};

#define ASSERT_PAGE_ALIGNED(address) ASSERT(Page::is_aligned(address));
#define ASSERT_HUGE_PAGE_ALIGNED(address) ASSERT(Page::is_huge_aligned(address));
}
