#include "TestRunner.h"

#ifdef _MSC_VER
int __builtin_ffsl(size_t value)
{
    unsigned long index = 0;
    Assert::that(static_cast<bool>(_BitScanForward64(&index, value))).is_true();
    
    // ffsl returns index + 1 in case a set bit was found, 0 otherwise
    return index + 1;
}
#endif

#include "Common/DynamicBitArray.h"

TEST(SetRangeToBool) {
    kernel::DynamicBitArray array(128);

    array.set_range_to(0, 65, true);

    for (size_t i = 0; i < 65; ++i)
        Assert::that(array.bit_at(i)).is_true();

    for (size_t i = 65; i < 128; ++i)
        Assert::that(array.bit_at(i)).is_false();
}

TEST(SetRangeToNumber) {
    kernel::DynamicBitArray array(8 + 16 + 32 + 64);

    array.set_range_to_number(0, 8, 0xDE);
    array.set_range_to_number(8, 16, 0xADBE);
    array.set_range_to_number(24, 32, 0xEFCABABE);
    array.set_range_to_number(56, 64, 0xDEADC0DEDEADBEEF);

    Assert::that(array.range_as_number(0, 8)).is_equal(0xDE);
    Assert::that(array.range_as_number(8, 16)).is_equal(0xADBE);
    Assert::that(array.range_as_number(24, 32)).is_equal(0xEFCABABE);
    Assert::that(array.range_as_number(56, 64)).is_equal(0xDEADC0DEDEADBEEF);

    array.set_range_to_number(62, 1, 0b1);
    array.set_range_to_number(63, 2, 0b10);
    array.set_range_to_number(65, 3, 0b101);
    array.set_range_to_number(0, 17, 0b1111000011101);

    Assert::that(array.range_as_number(62, 1)).is_equal(0b1);
    Assert::that(array.range_as_number(63, 2)).is_equal(0b10);
    Assert::that(array.range_as_number(65, 3)).is_equal(0b101);
    Assert::that(array.range_as_number(0, 17)).is_equal(0b1111000011101);
}

TEST(FindBit) {
    kernel::DynamicBitArray array(129);

    array.set_range_to(0, 129, true);
    array.set_bit(128, false);
    Assert::that(array.find_bit(false).value_or(-1)).is_equal(128);

    array.set_range_to(0, 129, false);
    array.set_bit(128, true);
    Assert::that(array.find_bit(true).value_or(-1)).is_equal(128);

    array.set_range_to(0, 129, false);
    Assert::that(array.find_bit(true).value_or(-1)).is_equal(-1);

    array.set_range_to(0, 129, true);
    Assert::that(array.find_bit(false).value_or(-1)).is_equal(-1);

    // good hint
    array.set_bit(63, false);
    array.set_bit(128, false);
    Assert::that(array.find_bit(false, 128).value_or(-1)).is_equal(128);

    // bad hint
    array.set_bit(128, true);
    Assert::that(array.find_bit(false, 128).value_or(-1)).is_equal(63);

    array.set_range_to(0, 129, false);

    // good hint
    array.set_bit(63, true);
    array.set_bit(126, true);
    Assert::that(array.find_bit(true, 64).value_or(-1)).is_equal(126);

    // bad hint
    array.set_bit(126, false);
    Assert::that(array.find_bit(true, 64).value_or(-1)).is_equal(63);
}

TEST(FindRange) {
    kernel::DynamicBitArray array(130);

    array.set_range_to(0, 129, true);

    Assert::that(array.find_range(129, true).value_or(-1)).is_equal(0);
    Assert::that(array.find_range(2, false).value_or(-1)).is_equal(-1);
    Assert::that(array.find_range(1, false).value_or(-1)).is_equal(129);
}
