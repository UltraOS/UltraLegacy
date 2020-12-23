#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/Runtime.h"

namespace kernel {

class Page {
public:
    static constexpr size_t size = 4096;

#ifdef ULTRA_32
    static constexpr size_t huge_size = 4 * MB;
    static constexpr ptr_t alignment_mask = 0xFFFFF000;
    static constexpr ptr_t huge_alignment_mask = 0xFFFFFFFF - huge_size + 1;
#elif defined(ULTRA_64)
    static constexpr size_t huge_size = 2 * MB;
    static constexpr ptr_t alignment_mask = 0xFFFFFFFFFFFFF000;
    static constexpr ptr_t huge_alignment_mask = 0xFFFFFFFFFFFFFFFF - huge_size + 1;
#endif

    explicit Page(Address physical_address);

    Address address() const;

    template <ptr_t AlignmentMask, size_t PageSize>
    static constexpr size_t round_up_custom(size_t size)
    {
        if (size == 0)
            return PageSize;

        return size & ~AlignmentMask ? (size + PageSize) & AlignmentMask : size;
    }

    template <ptr_t AlignmentMask>
    static constexpr size_t round_down_custom(size_t size)
    {
        return size & ~AlignmentMask ? size & AlignmentMask : size;
    }

    static constexpr size_t round_up(size_t size)
    {
        return round_up_custom<Page::alignment_mask, Page::size>(size);
    }

    static constexpr size_t round_up_huge(size_t size)
    {
        return round_up_custom<Page::huge_alignment_mask, Page::huge_size>(size);
    }

    static constexpr size_t round_down(size_t size)
    {
        return round_down_custom<Page::alignment_mask>(size);
    }

    static constexpr size_t round_down_huge(size_t size)
    {
        return round_down_custom<Page::huge_alignment_mask>(size);
    }

    static constexpr bool is_aligned(Address address)
    {
        return (address % size) == 0;
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
