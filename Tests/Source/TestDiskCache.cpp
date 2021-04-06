#include "TestRunner.h"

//#define private public
#include "FileSystem/DiskCache.h"
#include "FileSystem/DiskCache.cpp"

TEST(BlockToFirstLBA) {
    kernel::StorageDevice::Info test_info;
    test_info.logical_block_count = 100;
    test_info.optimal_read_size = 4096;
    kernel::StorageDevice drive3000;

    // Simplest case, logical sector == fs block
    test_info.logical_block_size = 512;
    kernel::StorageDevice::set_info(test_info);
    kernel::DiskCache cache1(drive3000, { 0, 100 }, 512, 9999);

    Assert::that(cache1.block_to_first_lba(0)).is_equal(0);
    Assert::that(cache1.block_to_first_lba(1)).is_equal(1);
    Assert::that(cache1.block_to_first_lba(2)).is_equal(2);
    Assert::that(cache1.block_to_first_lba(3)).is_equal(3);
    Assert::that(cache1.block_to_first_lba(4)).is_equal(4);

    // Logical sector > fs block
    test_info.logical_block_size = 4096;
    kernel::StorageDevice::set_info(test_info);
    kernel::DiskCache cache2(drive3000, { 0, 100 }, 512, 9999);

    Assert::that(cache2.block_to_first_lba(0)).is_equal(0);
    Assert::that(cache2.block_to_first_lba(4)).is_equal(0);
    Assert::that(cache2.block_to_first_lba(7)).is_equal(0);
    Assert::that(cache2.block_to_first_lba(8)).is_equal(1);
    Assert::that(cache2.block_to_first_lba(25)).is_equal(3);

    // FS block > logical sector (most common case)
    test_info.logical_block_size = 512;
    kernel::StorageDevice::set_info(test_info);
    kernel::DiskCache cache3(drive3000, { 33, 133 }, 8192, 9999);

    size_t blocks_per_lba = 8192 / 512;

    Assert::that(cache3.block_to_first_lba(0)).is_equal(33 + 0);
    Assert::that(cache3.block_to_first_lba(4)).is_equal(33 + blocks_per_lba * 4);
    Assert::that(cache3.block_to_first_lba(7)).is_equal(33 + blocks_per_lba * 7);
    Assert::that(cache3.block_to_first_lba(8)).is_equal(33 + blocks_per_lba * 8);
    Assert::that(cache3.block_to_first_lba(25)).is_equal(33 + blocks_per_lba * 25);
}
