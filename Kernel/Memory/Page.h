#pragma once

#include "Common/Types.h"
#include "Core/Runtime.h"

namespace kernel {

class Page {
public:
    static constexpr size_t  size           = 4096;
    static constexpr Address alignment_mask = 0xFFFFF000;

    Page(Address physical_address);

    Address address() const;

    static constexpr bool is_aligned(Address address) { return (address % size) == 0; }

    ~Page();

private:
    Address m_physical_address;
};

#define ASSERT_PAGE_ALIGNED(address) ASSERT(Page::is_aligned(address));
}
