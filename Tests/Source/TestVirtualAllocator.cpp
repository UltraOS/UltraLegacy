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
    Assert::that(Page::round_up(Page::size + 1)).is_equal(Page::size * 2);
    Assert::that(Page::round_up(1)).is_equal(Page::size);
    Assert::that(Page::round_up(Page::size - 1)).is_equal(Page::size);
}

TEST(PageRoundDown) {
    using Page = kernel::Page;

    Assert::that(Page::round_down(0)).is_equal(0);
    Assert::that(Page::round_down(Page::size + 1)).is_equal(Page::size);
    Assert::that(Page::round_down(1)).is_equal(0);
    Assert::that(Page::round_down(Page::size - 1)).is_equal(0);
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
