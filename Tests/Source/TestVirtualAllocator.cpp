#include <type_traits>
#include <string>

template <typename RangeT>
std::enable_if_t<std::is_same_v<RangeT, decltype(RangeT::create_empty_at(0ull))>, std::string> serialize(RangeT range)
{
    std::string repr;

    repr += "begin: ";
    repr += std::to_string(range.begin());
    repr += " end: ";
    repr += std::to_string(range.end());

    return repr;
}

#include "TestRunner.h"

#define private public
#include "Memory/VirtualAllocator.h"
#include "Memory/VirtualAllocator.cpp"
#undef private

template <typename T>
constexpr auto as_ptr_t(T value)
{
    return static_cast<kernel::ptr_t>(value);
}

TEST(PageIsAligned) {
    using Page = kernel::Page;

    Assert::that(Page::is_aligned(as_ptr_t(0))).is_true();
    Assert::that(Page::is_aligned(as_ptr_t(1))).is_false();
    Assert::that(Page::is_aligned(as_ptr_t(Page::size))).is_true();
    Assert::that(Page::is_aligned(as_ptr_t(Page::huge_size))).is_true();

    Assert::that(Page::is_huge_aligned(as_ptr_t(0))).is_true();
    Assert::that(Page::is_huge_aligned(as_ptr_t(1))).is_false();
    Assert::that(Page::is_huge_aligned(as_ptr_t(Page::size))).is_false();
    Assert::that(Page::is_huge_aligned(as_ptr_t(Page::huge_size))).is_true();
}

TEST(PageRoundUp) {
    using Page = kernel::Page;

    Assert::that(Page::round_up(0)).is_equal(Page::size);
    Assert::that(Page::round_up(1)).is_equal(Page::size);
    Assert::that(Page::round_up(Page::size + 1)).is_equal(Page::size * 2);
    Assert::that(Page::round_up(Page::size - 1)).is_equal(Page::size);
    Assert::that(Page::round_up(Page::size)).is_equal(Page::size);
}

TEST(PageRoundDown) {
    using Page = kernel::Page;

    Assert::that(Page::round_down(0)).is_equal(0);
    Assert::that(Page::round_down(1)).is_equal(0);
    Assert::that(Page::round_down(Page::size - 1)).is_equal(0);
    Assert::that(Page::round_down(Page::size + 1)).is_equal(Page::size);
    Assert::that(Page::round_down(Page::size)).is_equal(Page::size);
}

TEST(RangeClipping) {
    using Range = kernel::VirtualAllocator::Range;

    Range base(as_ptr_t(0x0000), 0x1000);

    Assert::that(base.clipped_with_begin(0x100)).is_equal(Range::from_two_pointers(as_ptr_t(0x100), 0x1000));
    Assert::that(base.clipped_with_begin(as_ptr_t(0x000))).is_equal(base);
    Assert::that(base.clipped_with_end(0x200)).is_equal({ as_ptr_t(0x0), 0x200 });
    Assert::that(base.clipped_with_end(0x1000)).is_equal(base);
}

TEST(RangeAlign) {
    using Range = kernel::VirtualAllocator::Range;
    using Page = kernel::Page;

    // 0x0001 - 0x1000 range
    auto range1 = Range(0x1, Page::size);

    Assert::that(range1.is_aligned_to(Page::size)).is_false();
    Assert::that(range1.aligned_to(Page::size)).is_equal({ Page::size, 1 });

    // 0x0001 - 0x2000 range
    auto range2 = Range(0x1, 2 * Page::size);

    Assert::that(range2.is_aligned_to(Page::size)).is_false();
    Assert::that(range2.aligned_to(Page::size)).is_equal({ Page::size, Page::size + 1 });

    // 0x0000 - 0x2000 range
    auto range3 = Range(as_ptr_t(0x0), 2 * Page::size);

    Assert::that(range3.is_aligned_to(Page::size)).is_true();
    Assert::that(range3.aligned_to(Page::size)).is_equal(range3);

    // 0x0000 - 0x0000 range
    auto range4 = Range(as_ptr_t(0x0), 0x0);

    Assert::that(range4.is_aligned_to(Page::size)).is_true();
    Assert::that(range4.aligned_to(Page::size)).is_equal(range4);

    // 0x0001 - HUGE_PAGE_SIZE * 2 range
    auto range5 = Range(0x1, Page::huge_size * 2);

    Assert::that(range5.is_aligned_to(Page::huge_size)).is_false();
    Assert::that(range5.aligned_to(Page::huge_size)).is_equal({ Page::huge_size, Page::huge_size + 1 });

    // overflow test
    if constexpr (sizeof(void*) == 4) {
        auto range6 = Range(0xFFFFFFF0, 0xF);

        Assert::that(range6.is_aligned_to(Page::size)).is_false();

        // Should return an empty range as this alignment would cause overflow
        Assert::that(range6.aligned_to(Page::size)).is_equal({});
    } else {
        auto range6 = Range(0xFFFFFFFFFFFFFFF0, 0xF);

        Assert::that(range6.is_aligned_to(Page::size)).is_false();

        // Should return an empty range as this alignment would cause overflow
        Assert::that(range6.aligned_to(Page::size)).is_equal({});
    }

    // 0x0001 - 0x0F00 range
    auto range7 = Range(as_ptr_t(0x1), 0x0F00);

    Assert::that(range7.is_aligned_to(Page::size)).is_false();

    // Not enough length to handle alignment
    Assert::that(range7.aligned_to(Page::size)).is_equal({});

    // 0x0001 - 2 pages range
    auto range8 = Range(0x1, 2 * Page::size);
    range8.align_to(Page::size);
    Assert::that(range8).is_equal({ Page::size, Page::size + 1 });

    // 0x0000 - 2 pages range (no alignment needed)
    auto range9 = Range(as_ptr_t(0x0), 2 * Page::size);
    range9.align_to(Page::size);
    Assert::that(range9).is_equal({ as_ptr_t(0x0), 2 * Page::size });
}

TEST(SizedAllocations) {
    using Range = kernel::VirtualAllocator::Range;

    kernel::VirtualAllocator allocator(as_ptr_t(0x0000) , 0x2000);

    Assert::that(allocator.allocate(0x1000)).is_equal({ as_ptr_t(0x0000), 0x1000 });
    Assert::that(allocator.allocate(0x1000)).is_equal({ as_ptr_t(0x1000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(0x0001);
    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ as_ptr_t(0x0000), 0x2000 });

    bool did_fail = false;
    try {
        allocator.allocate(0x1000);
    } catch(...) {
        did_fail = true;
    }

    Assert::that(did_fail).is_true();
}

TEST(SpecificAllocations) {
    using Range = kernel::VirtualAllocator::Range;

    kernel::VirtualAllocator allocator(as_ptr_t(0x0000), 0x3000);

    Assert::that(allocator.allocate({ as_ptr_t(0x0000), 0x1000 })).is_equal({ as_ptr_t(0x0000), 0x1000 });
    Assert::that(allocator.allocate({ as_ptr_t(0x1050), 0x950 })).is_equal({ as_ptr_t(0x1000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(1);
    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ as_ptr_t(0x0000), 0x2000 });

    bool did_fail = false;
    try {
        allocator.allocate({ as_ptr_t(0x0000), 0x1000 });
    }
    catch (...) {
        did_fail = true;
    }

    Assert::that(did_fail).is_true();
}

TEST(TripleMergeSpecific) {
    using Range = kernel::VirtualAllocator::Range;

    kernel::VirtualAllocator allocator(as_ptr_t(0x0000), 0x3000);

    Assert::that(allocator.allocate({ as_ptr_t(0x0000), 0x1000 })).is_equal({ as_ptr_t(0x0000), 0x1000 });
    Assert::that(allocator.allocate({ as_ptr_t(0x2000), 0x1000 })).is_equal({ as_ptr_t(0x2000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(2);

    Assert::that(allocator.allocate({ as_ptr_t(0x1000), 0x1000 })).is_equal({ as_ptr_t(0x1000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(1);

    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ as_ptr_t(0x0000), 0x3000 });
}

TEST(SizedMergeCase1) {
    // allocate 0x0000 - 0x1000
    // allocate 0x1000 - 0x2000
    // gets merged into 0x0000 - 0x2000
    // allocate 0x2000 - 0x3000
    // gets merged into 0x0000 - 0x3000
    // deallocate 0x1000 - 0x2000
    // gets separated into 0x0000 - 0x1000 & 0x2000 - 0x3000
    // allocate 0x1000 - 0x2000
    // gets merged into 0x0000 - 0x3000

    using Range = kernel::VirtualAllocator::Range;

    kernel::VirtualAllocator allocator(as_ptr_t(0), 0x3000);

    Assert::that(allocator.allocate(0x1000)).is_equal({ as_ptr_t(0x0000), 0x1000 });
    Assert::that(allocator.allocate(0x1000)).is_equal({ as_ptr_t(0x1000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(1);
    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ as_ptr_t(0x0000), 0x2000 });

    Assert::that(allocator.allocate(0x1000)).is_equal({ as_ptr_t(0x2000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(1);
    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ as_ptr_t(0x0000), 0x3000 });

    allocator.deallocate({ as_ptr_t(0x1000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(2);
    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ as_ptr_t(0x0000), 0x1000 });
    Assert::that(*(++allocator.m_allocated_ranges.begin())).is_equal({ as_ptr_t(0x2000), 0x1000 });

    Assert::that(allocator.allocate(0x1000)).is_equal({ as_ptr_t(0x1000), 0x1000 });

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(1);
    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ as_ptr_t(0x0000), 0x3000 });
}

TEST(AlignedAllocations) {
    using Range = kernel::VirtualAllocator::Range;
    using Page = kernel::Page;

    // 0x1000 - 2 huge pages range
    kernel::VirtualAllocator allocator(Page::size, Page::huge_size * 2);

    auto range = allocator.allocate(Page::size, Page::huge_size);

    Assert::that(range).is_equal({ Page::huge_size, Page::size });

    // Now that we allocate a page aligned to huge page we have something like this
    // 0x1000 -> huge page ... huge page + page -> huge_page * 2

    // Allocate the entire lower range, aka 0x1000 - huge page
    auto range2 = allocator.allocate(Page::size * (Page::huge_size / Page::size) - Page::size);

    Assert::that(allocator.m_allocated_ranges.size()).is_equal(1);
    Assert::that(*allocator.m_allocated_ranges.begin()).is_equal({ 0x1000, Page::huge_size });
}
