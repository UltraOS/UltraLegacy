#pragma once

#include "Common/Types.h"
#include "Core/Runtime.h"

namespace kernel {

class Page {
public:
#ifdef ULTRA_64
    static constexpr size_t  huge_size      = 2 * MB;
#endif
    static constexpr size_t  size           = 4096;
    static constexpr Address alignment_mask = 0xFFFFF000;

    Page(Address physical_address);

    Address address() const;

    static constexpr bool is_aligned(Address address) { return (address % size) == 0; }
#ifdef ULTRA_64
    static constexpr bool is_huge_aligned(Address address) { return (address % (2 * MB)) == 0; }
#endif

    ~Page();

private:
    Address m_physical_address;
};

#define ASSERT_PAGE_ALIGNED(address) ASSERT(Page::is_aligned(address));

#ifdef ULTRA_64
#define ASSERT_HUGE_PAGE_ALIGNED(address) ASSERT(Page::is_huge_aligned(address));
#endif
}
