#include "TestRunner.h"

#define private public
#include "Memory/BootAllocator.h"
#include "Memory/BootAllocator.cpp"

template <typename T>
constexpr auto as_ptr_t(T number) {
    return static_cast<kernel::ptr_t>(number);
}

// I haven't written a ReserveAt test because both reserve_contiguous & reserve_at use one single function
// That said maybe I should write one later? Just can't think of a separate test case for that for now.
// Anyways, this test looks good enough and covers most (if not all?) edge cases.
TEST(ReserveContiguous) {
    using Range = kernel::MemoryMap::PhysicalRange;
    using RangeType = kernel::MemoryMap::PhysicalRange::Type;

    kernel::MemoryMap map;

    static constexpr size_t buffer_size = 4096;
    auto* range_buffer = new uint8_t[buffer_size];
    map.set_range_buffer(range_buffer, buffer_size);

    map.emplace_range(as_ptr_t(0x0000), 0x1000, RangeType::GENERIC_BOOTALLOC_RESERVED);
    map.emplace_range(0x1000ull, 0x1000, RangeType::FREE); // gets backwards merged with previous

    map.emplace_range(0x4000ull, 0x1000, RangeType::RESERVED);
    map.emplace_range(0x5000ull, 0x1000, RangeType::FREE); // gets turned into GENERIC_BOOTALLOC_RESERVED
    map.emplace_range(0x6000ull, 0x1000, RangeType::BAD);

    map.emplace_range(0x7000ull, 0x1000, RangeType::RESERVED);
    map.emplace_range(0x8000ull, 0x2000, RangeType::FREE); // gets turned into GENERIC_BOOTALLOC_RESERVED, upper part is kept as is
    map.emplace_range(0xA000ull, 0x1000, RangeType::BAD);

    map.emplace_range(0xB000ull, 0x1000, RangeType::RESERVED);
    map.emplace_range(0xC000ull, 0x2000, RangeType::FREE); // gets turned into GENERIC_BOOTALLOC_RESERVED, lower part is kept as is
    map.emplace_range(0xE000ull, 0x1000, RangeType::BAD);

    // allocate middle, lhs and rhs should still remain
    map.emplace_range(0x10000ull, 0x3000, RangeType::FREE);

    kernel::BootAllocator ba(map);

    // should get backwards merged with the previous range
    Assert::that(ba.reserve_contiguous(1, 0x1000ull, 0x2000ull).raw()).is_equal(0x1000ull);
    Assert::that(ba.reserve_contiguous(1, 0x1000ull, 0xF0000ull).raw()).is_equal(0x5000ull);
    Assert::that(ba.reserve_contiguous(1, 0x1000ull, 0xF0000ull).raw()).is_equal(0x8000ull);
    Assert::that(ba.reserve_contiguous(1, 0xD000ull, 0xF0000ull).raw()).is_equal(0xD000ull);
    Assert::that(ba.reserve_contiguous(1, 0x11000ull,0xF0000ull).raw()).is_equal(0x11000ull);

    Assert::that(ba.m_memory_map.entry_count()).is_equal(15);

    Assert::that(ba.m_memory_map.at(0)).is_equal(Range(as_ptr_t(0x0000), 0x2000, RangeType::GENERIC_BOOTALLOC_RESERVED));
    Assert::that(ba.m_memory_map.at(1)).is_equal(Range(0x4000ull, 0x1000, RangeType::RESERVED));
    Assert::that(ba.m_memory_map.at(2)).is_equal(Range(0x5000ull, 0x1000, RangeType::GENERIC_BOOTALLOC_RESERVED));
    Assert::that(ba.m_memory_map.at(3)).is_equal(Range(0x6000ull, 0x1000, RangeType::BAD));
    Assert::that(ba.m_memory_map.at(4)).is_equal(Range(0x7000ull, 0x1000, RangeType::RESERVED));
    Assert::that(ba.m_memory_map.at(5)).is_equal(Range(0x8000ull, 0x1000, RangeType::GENERIC_BOOTALLOC_RESERVED));
    Assert::that(ba.m_memory_map.at(6)).is_equal(Range(0x9000ull, 0x1000, RangeType::FREE));
    Assert::that(ba.m_memory_map.at(7)).is_equal(Range(0xA000ull, 0x1000, RangeType::BAD));
    Assert::that(ba.m_memory_map.at(8)).is_equal(Range(0xB000ull, 0x1000, RangeType::RESERVED));
    Assert::that(ba.m_memory_map.at(9)).is_equal(Range(0xC000ull, 0x1000, RangeType::FREE));
    Assert::that(ba.m_memory_map.at(10)).is_equal(Range(0xD000ull, 0x1000, RangeType::GENERIC_BOOTALLOC_RESERVED));
    Assert::that(ba.m_memory_map.at(11)).is_equal(Range(0xE000ull, 0x1000, RangeType::BAD));
    Assert::that(ba.m_memory_map.at(12)).is_equal(Range(0x10000ull, 0x1000, RangeType::FREE));
    Assert::that(ba.m_memory_map.at(13)).is_equal(Range(0x11000ull, 0x1000, RangeType::GENERIC_BOOTALLOC_RESERVED));
    Assert::that(ba.m_memory_map.at(14)).is_equal(Range(0x12000ull, 0x1000, RangeType::FREE));
}
