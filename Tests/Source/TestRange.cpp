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

#include "Memory/Range.h"
#include "Memory/Page.h"

template <typename T>
constexpr auto as_ptr_t(T value)
{
    return static_cast<kernel::ptr_t>(value);
}

TEST(Clipping) {
    using Range = kernel::Range;

    Range base(as_ptr_t(0x0000), 0x1000);

    Assert::that(base.clipped_with_begin(0x100)).is_equal(Range::from_two_pointers(as_ptr_t(0x100), 0x1000));
    Assert::that(base.clipped_with_begin(as_ptr_t(0x000))).is_equal(base);
    Assert::that(base.clipped_with_end(0x200)).is_equal({ as_ptr_t(0x0), 0x200 });
    Assert::that(base.clipped_with_end(0x1000)).is_equal(base);
}

TEST(Align) {
    using Range = kernel::Range;
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
    }
    else {
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

TEST(Contains) {
    using Range = kernel::Range;

    // 0x1000 - 0x2000 range
    auto range = Range(as_ptr_t(0x1000), 0x1000);

    Assert::that(range.contains(as_ptr_t(0x1000))).is_true();
    Assert::that(range.contains(0x2000 - 1)).is_true();
    Assert::that(range.contains(0x2000)).is_false();
    Assert::that(range.contains(range)).is_true();
    Assert::that(range.contains(Range::from_two_pointers(range.begin(), range.end() + 1))).is_false();
    Assert::that(range.contains(Range::from_two_pointers(range.begin() - 1, range.end()))).is_false();
}
