#include <type_traits>
#include <string>

template <typename RangeT>
std::enable_if_t<std::is_same_v<typename RangeT::Type, decltype(RangeT::Type::FREE)>, std::string> serialize(RangeT range)
{
    std::string repr;

    repr += "begin: ";
    repr += std::to_string(range.begin());
    repr += " end: ";
    repr += std::to_string(range.end());
    repr += " type: ";
    repr += range.type_as_string().data();

    return repr;
}

#include "TestRunner.h"

extern "C" {
    uint8_t memory_map_entries_buffer[4096] = {};
}

#define private public
#include "Memory/MemoryMap.h"
#include "Memory/MemoryMap.cpp"

template <typename T>
constexpr auto as_ptr_t(T number) {
    return static_cast<kernel::ptr_t>(number);
}

TEST(ShatterCase1) {
    // Both ranges are the same type, so they are supposed to end up merged together into 1 range
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(as_ptr_t(0x0000), 0x1000, RangeType::FREE);
    auto r1 = Range(0x500ull, 0x1000, RangeType::FREE);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(as_ptr_t(0x0000), 0x1500, RangeType::FREE));
}

TEST(ShatterCase2) {
    // the lhs range is free and rhs is reserved while containing an overlapping range
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(as_ptr_t(0x0000), 0x1000, RangeType::FREE);
    auto r1 = Range(0x500ull, 0x1000, RangeType::BAD);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(as_ptr_t(0x0000), 0x500, RangeType::FREE));
    Assert::that(ranges[1]).is_equal(Range(0x500ull, 0x1000, RangeType::BAD));
}

TEST(ShatterCase3) {
    // the rhs range is free and lhs is reserved while containing an overlapping range
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(as_ptr_t(0x0000), 0x1000, RangeType::BAD);
    auto r1 = Range(0x500ull, 0x1000, RangeType::FREE);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(as_ptr_t(0x0000), 0x1000, RangeType::BAD));
    Assert::that(ranges[1]).is_equal(Range(0x1000ull, 0x500, RangeType::FREE));
}

TEST(ShatterCase4) {
    // ranges are equal but have different types, first range gets cut out
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(as_ptr_t(0x0000), 0x1000, RangeType::FREE);
    auto r1 = Range(as_ptr_t(0x0000), 0x1000, RangeType::BAD);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(as_ptr_t(0x0000), 0, RangeType::FREE));
    Assert::that(ranges[1]).is_equal(Range(as_ptr_t(0x0000), 0x1000, RangeType::BAD));
}

TEST(ShatterCase5) {
    // ranges are equal but have different types, second range gets cut out
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(as_ptr_t(0x0000), 0x1000, RangeType::BAD);
    auto r1 = Range(as_ptr_t(0x0000), 0x1000, RangeType::FREE);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(as_ptr_t(0x0000), 0x1000, RangeType::BAD));
    Assert::that(ranges[1]).is_equal(Range(0x1000ull, 0, RangeType::FREE));
}

TEST(ShatterCase6) {
    // the second range is a subrange of the first one, but same type
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(as_ptr_t(0x0000), 0x3000, RangeType::FREE);
    auto r1 = Range(0x1000ull, 0x1000, RangeType::FREE);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(as_ptr_t(0x0000), 0x3000, RangeType::FREE));
    Assert::that(ranges[1]).is_equal({});
}

TEST(ShatterCase7) {
    // the second range is a subrange of the first one and a different type as well
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(as_ptr_t(0x0000), 0x3000, RangeType::FREE);
    auto r1 = Range(0x1000ull, 0x1000, RangeType::RESERVED);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(as_ptr_t(0x0000), 0x1000, RangeType::FREE));
    Assert::that(ranges[1]).is_equal(Range(0x1000ull, 0x1000, RangeType::RESERVED));
    Assert::that(ranges[2]).is_equal(Range(0x2000ull, 0x1000, RangeType::FREE));
}

TEST(ShatterCase8) {
    // the second range swallows the first entirely
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    auto r = Range(0x15000ull, 0x2123, RangeType::BAD);
    auto r1 = Range(0x16000ull, 0x1000, RangeType::FREE);

    Assert::that(r.overlaps(r1)).is_true();

    auto ranges = r.shatter_against(r1);

    Assert::that(ranges[0]).is_equal(Range(0x15000ull, 0x2123, RangeType::BAD));
    Assert::that(ranges[1]).is_equal({});
    Assert::that(ranges[2]).is_equal({});
}

TEST(CopyAligned) {
    using BIOSRange = kernel::E820MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    kernel::MemoryMap clean_map;

    static constexpr size_t buffer_size = 4096;
    auto* range_buffer = new uint8_t[buffer_size];
    clean_map.set_range_buffer(range_buffer, buffer_size);

    kernel::E820MemoryMap bios_map;
    auto* bios_ranges = new BIOSRange[5];

    bios_map.entries = bios_ranges;
    bios_map.entry_count = 5;

    // we expect this to turn into 0x1000 - 0x2000
    bios_ranges[0].base_address = 0x123;
    bios_ranges[0].length = 0x1000 + 0x1000 - 0x123;
    bios_ranges[0].type = BIOSRange::Type::FREE;

    // we expect this to get eliminated as it cannot be aligned properly
    bios_ranges[1].base_address = 0x2123;
    bios_ranges[1].length = 0x1000 - 0x124;
    bios_ranges[1].type = BIOSRange::Type::FREE;

    // this shouldn't be touched since its a reserved range
    bios_ranges[2].base_address = 0x3123;
    bios_ranges[2].length = 0x1000;
    bios_ranges[2].type = BIOSRange::Type::RESERVED;

    // we expect this to get eliminated since it ends up as 0x5000 - 0x5123 after alignment (under page size)
    bios_ranges[3].base_address = 0x4123;
    bios_ranges[3].length = 0x1000;
    bios_ranges[3].type = BIOSRange::Type::FREE;

    // aligned properly but weird size, expect this to turn into 0x8000 - 0x9000
    bios_ranges[4].base_address = 0x8000;
    bios_ranges[4].length = 0x1FFF;
    bios_ranges[4].type = BIOSRange::Type::FREE;

    clean_map.copy_aligned_from(bios_map);

    Assert::that(clean_map.entry_count()).is_equal(3);

    Assert::that(clean_map.at(0)).is_equal({ 0x1000ull, 0x1000, RangeType::FREE });
    Assert::that(clean_map.at(1)).is_equal({ 0x3123ull, 0x1000, RangeType::RESERVED });
    Assert::that(clean_map.at(2)).is_equal({ 0x8000ull, 0x1000, RangeType::FREE });

    delete[] bios_ranges;
    delete[] range_buffer;
}

TEST(SortByAddress) {
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    kernel::MemoryMap clean_map;

    static constexpr size_t buffer_size = 4096;
    auto* range_buffer = new uint8_t[buffer_size];
    clean_map.set_range_buffer(range_buffer, buffer_size);

    clean_map.emplace_range(0x5000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0x3000ull, 0x1000, RangeType::RESERVED);
    clean_map.emplace_range(as_ptr_t(0x0000), 0xF000, RangeType::FREE);
    clean_map.emplace_range(0x4000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0x1000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0x2000ull, 0x1000, RangeType::BAD);

    clean_map.sort_by_address();

    Assert::that(clean_map.at(0)).is_equal({ as_ptr_t(0x0000), 0xF000, RangeType::FREE });
    Assert::that(clean_map.at(1)).is_equal({ 0x1000ull, 0x1000, RangeType::FREE });
    Assert::that(clean_map.at(2)).is_equal({ 0x2000ull, 0x1000, RangeType::BAD });
    Assert::that(clean_map.at(3)).is_equal({ 0x3000ull, 0x1000, RangeType::RESERVED });
    Assert::that(clean_map.at(4)).is_equal({ 0x4000ull, 0x1000, RangeType::FREE });
    Assert::that(clean_map.at(5)).is_equal({ 0x5000ull, 0x1000, RangeType::FREE });

    delete[] range_buffer;
}

TEST(EraseAt) {
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    kernel::MemoryMap clean_map;

    static constexpr size_t buffer_size = 4096;
    auto* range_buffer = new uint8_t[buffer_size];
    clean_map.set_range_buffer(range_buffer, buffer_size);

    clean_map.emplace_range(as_ptr_t(0x0000), 0x1000, RangeType::RECLAIMABLE);
    clean_map.emplace_range(0x1000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0x2000ull, 0x1000, RangeType::RESERVED);
    clean_map.emplace_range(0x3000ull, 0x1000, RangeType::BAD);

    Assert::that(clean_map.entry_count()).is_equal(4);

    clean_map.erase_range_at(1);

    Assert::that(clean_map.entry_count()).is_equal(3);
    Assert::that(clean_map.at(0)).is_equal({ as_ptr_t(0x0000), 0x1000, RangeType::RECLAIMABLE });
    Assert::that(clean_map.at(1)).is_equal({ 0x2000ull, 0x1000, RangeType::RESERVED });
    Assert::that(clean_map.at(2)).is_equal({ 0x3000ull, 0x1000, RangeType::BAD });

    clean_map.erase_range_at(0);

    Assert::that(clean_map.entry_count()).is_equal(2);
    Assert::that(clean_map.at(0)).is_equal({ 0x2000ull, 0x1000, RangeType::RESERVED });
    Assert::that(clean_map.at(1)).is_equal({ 0x3000ull, 0x1000, RangeType::BAD });

    clean_map.erase_range_at(1);

    Assert::that(clean_map.entry_count()).is_equal(1);
    Assert::that(clean_map.at(0)).is_equal({ 0x2000ull, 0x1000, RangeType::RESERVED });

    clean_map.erase_range_at(0);
    Assert::that(clean_map.entry_count()).is_equal(0);

    delete[] range_buffer;
}

TEST(EmplaceAt) {
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    kernel::MemoryMap clean_map;

    static constexpr size_t buffer_size = 4096;
    auto* range_buffer = new uint8_t[buffer_size];
    clean_map.set_range_buffer(range_buffer, buffer_size);

    clean_map.emplace_range(0x1000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0x3000ull, 0x1000, RangeType::BAD);

    Assert::that(clean_map.entry_count()).is_equal(2);

    clean_map.emplace_range_at(1, 0x2000ull, 0x1000, RangeType::RESERVED);

    Assert::that(clean_map.entry_count()).is_equal(3);
    Assert::that(clean_map.at(0)).is_equal({ 0x1000ull, 0x1000, RangeType::FREE });
    Assert::that(clean_map.at(1)).is_equal({ 0x2000ull, 0x1000, RangeType::RESERVED });
    Assert::that(clean_map.at(2)).is_equal({ 0x3000ull, 0x1000, RangeType::BAD });

    clean_map.emplace_range_at(0, as_ptr_t(0x0000), 0x1000, RangeType::RECLAIMABLE);

    Assert::that(clean_map.entry_count()).is_equal(4);
    Assert::that(clean_map.at(0)).is_equal({ as_ptr_t(0x0000), 0x1000, RangeType::RECLAIMABLE });
    Assert::that(clean_map.at(1)).is_equal({ 0x1000ull, 0x1000, RangeType::FREE });
    Assert::that(clean_map.at(2)).is_equal({ 0x2000ull, 0x1000, RangeType::RESERVED });
    Assert::that(clean_map.at(3)).is_equal({ 0x3000ull, 0x1000, RangeType::BAD });

    delete[] range_buffer;
}

TEST(CorrectOverlapping) {
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    kernel::MemoryMap clean_map;

    static constexpr size_t buffer_size = 4096;
    auto* range_buffer = new uint8_t[buffer_size];
    clean_map.set_range_buffer(range_buffer, buffer_size);

    // these 4 should get merged into 0x0000 - 0x4000
    clean_map.emplace_range(as_ptr_t(0x0000), 0x2000, RangeType::FREE);
    clean_map.emplace_range(0x1000ull, 0x2000, RangeType::FREE);
    clean_map.emplace_range(0x2000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0x2000ull, 0x2000, RangeType::FREE);

    // these 2 should get merged into 0x5000 - 0x7000
    clean_map.emplace_range(0x5000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0x6000ull, 0x1000, RangeType::FREE);

    // a possible edge case for the previous example. this should stay as is.
    clean_map.emplace_range(0x7000ull, 0x500, RangeType::BAD);

    // the first one should get cut into 0x8000 - 0x9000, the second one stays the same
    clean_map.emplace_range(0x8000ull, 0x2000, RangeType::FREE);
    clean_map.emplace_range(0x9000ull, 0x1000, RangeType::RESERVED);

    // the second one should get cut into 0xB000 - 0xC000, the first one stays as is
    clean_map.emplace_range(0xA000ull, 0x1000, RangeType::BAD);
    clean_map.emplace_range(0xA000ull, 0x2000, RangeType::FREE);

    // the first one gets removed altogether, second stays the same
    clean_map.emplace_range(0xD000ull, 0x1000, RangeType::FREE);
    clean_map.emplace_range(0xD000ull, 0x1000, RangeType::BAD);

    // the second one gets removed altogether, first stays the same
    clean_map.emplace_range(0xF000ull, 0x1000, RangeType::BAD);
    clean_map.emplace_range(0xF000ull, 0x1000, RangeType::FREE);

    // bad range is right inside the free range, so they get split into 3 ranges
    clean_map.emplace_range(0x11000ull, 0x3000, RangeType::FREE);
    clean_map.emplace_range(0x12000ull, 0x1000, RangeType::BAD);

    // free range is right inside the bad range, we expect for it to get swallowed entirely
    clean_map.emplace_range(0x15000ull, 0x2123, RangeType::BAD);
    clean_map.emplace_range(0x16000ull, 0x1000, RangeType::FREE);

    // bad range is right inside the free range, and touches all free pages
    // both free ranges are too small to be saved (0xFFF and 0xFFF)
    clean_map.emplace_range(0x18000ull, 0x2000, RangeType::FREE);
    clean_map.emplace_range(0x18FFFull, 0x1001, RangeType::BAD);

    // these two are supposed to collapse into 1 range and get backwards-merged with the previous range
    clean_map.emplace_range(0x18FFFull + 0x1001, 0x1000, RangeType::RESERVED);
    clean_map.emplace_range(0x18FFFull + 0x1001, 0x1000, RangeType::BAD);

    // same as above but we leave a small range after
    clean_map.emplace_range(0x18FFFull + 0x2001, 0x1001, RangeType::RESERVED);
    clean_map.emplace_range(0x18FFFull + 0x2001, 0x1000, RangeType::BAD);

    clean_map.correct_overlapping_ranges();

    Assert::that(clean_map.entry_count()).is_equal(15);

    Assert::that(clean_map.at(0)).is_equal({  as_ptr_t(0x0000),  0x4000, RangeType::FREE     });
    Assert::that(clean_map.at(1)).is_equal({  0x5000ull,  0x2000, RangeType::FREE     });
    Assert::that(clean_map.at(2)).is_equal({  0x7000ull,  0x500,  RangeType::BAD      });
    Assert::that(clean_map.at(3)).is_equal({  0x8000ull,  0x1000, RangeType::FREE     });
    Assert::that(clean_map.at(4)).is_equal({  0x9000ull,  0x1000, RangeType::RESERVED });
    Assert::that(clean_map.at(5)).is_equal({  0xA000ull,  0x1000, RangeType::BAD      });
    Assert::that(clean_map.at(6)).is_equal({  0xB000ull,  0x1000, RangeType::FREE     });
    Assert::that(clean_map.at(7)).is_equal({  0xD000ull,  0x1000, RangeType::BAD      });
    Assert::that(clean_map.at(8)).is_equal({  0xF000ull,  0x1000, RangeType::BAD      });
    Assert::that(clean_map.at(9)).is_equal({  0x11000ull, 0x1000, RangeType::FREE     });
    Assert::that(clean_map.at(10)).is_equal({ 0x12000ull, 0x1000, RangeType::BAD      });
    Assert::that(clean_map.at(11)).is_equal({ 0x13000ull, 0x1000, RangeType::FREE     });
    Assert::that(clean_map.at(12)).is_equal({ 0x15000ull, 0x2123, RangeType::BAD      });
    Assert::that(clean_map.at(13)).is_equal({ 0x18FFFull, 0x3001, RangeType::BAD      });
    Assert::that(clean_map.at(14)).is_equal({ 0x1C000ull, 0x0001, RangeType::RESERVED });

    delete[] range_buffer;
}
