#pragma once

#include "Common/List.h"
#include "Common/Map.h"
#include "Drivers/Storage.h"

namespace kernel {

// A few things about the cache:
// - Cache behavior in respect to block size/fs block size:
//       If logical sector size is 512:
//           If FS block is under 4K:
//               If partition is aligned to FS blocks per page:
//                   Starting LBA is aligned down to 8 LBA boundary, and FS blocks per page are
//                   read from disk and cached. (e.g if it's a read of FS block 1 of size 1024 (LBA 2),
//                   it's aligned down to LBA 0 and blocks 0 -> 8 are read and FS blocks 0 -> 3 are cached)
//               If partition is not aligned to FS blocks per page:
//                   No alignment is performed and FS blocks per page are read and cached.
//           If FS block is over 4K:
//               Any read is done exactly as requested and no alignment or extra caching is performed.
//       If logical sector size is 4K:
//          Same rules as "logical sector size is 512" apply, except instead of reading multiple logical blocks only 1 is read
//          for cases where FS block size is under 4K. No alignment is performed.
//       Any other logical sector size:
//          Unimplemented.
//
// - Block cache eviction works based on the LRU rule. Aka the first block to be evicted is the block, which happens
//   to be the least recently used.
// - Actual allocated physical memory grows on demand, but never shrinks.
// - One cache block - one fs block unless fs block size is under 4K in which case one cache block stores N fs blocks
//   that add up to 4K.

class DiskCache {
public:
    DiskCache(StorageDevice& device, LBARange filesystem_lba_range, size_t filesystem_block_size, size_t block_capacity);

    void read_one(u64 block_index, size_t offset, size_t bytes, void* buffer);
    void write_one(u64 block_index, size_t offset, size_t bytes, void* buffer);

private:
    static constexpr size_t no_caching_required = 0;

    u64 block_to_first_lba(u64 block_index) const;
    u64 block_to_first_lba_within_fs(u64 block_index) const;
    LBARange block_to_lba_range(u64 block_index) const;

    struct CachedBlock : public StandaloneListNode<CachedBlock> {
        Address virtual_address_and_dirty_bit { nullptr };
        u64 first_block { 0 };

        void mark_dirty() { virtual_address_and_dirty_bit |= 1; }
        void mark_clean() { virtual_address_and_dirty_bit &= ~1; }
        bool is_dirty() const { return virtual_address_and_dirty_bit & 1; }
        Address virtual_address() const { return virtual_address_and_dirty_bit & ~1; }
    };

    CachedBlock* evict_one();
    Pair<CachedBlock*, size_t> cached_block(u64 block_index);
    Address allocate_next_block_buffer();

private:
    StorageDevice& m_device;
    LBARange m_fs_lba_range;
    size_t m_io_size { 0 };
    size_t m_logical_block_size { 0 };
    size_t m_fs_block_size { 0 };
    size_t m_fs_blocks_per_io { 0 };
    size_t m_capacity { 0 };
    PrivateVirtualRegion* m_region { nullptr };
    size_t m_offset_within_region { 0 };
    List<CachedBlock> m_cached_blocks;
    Map<u64, CachedBlock*> m_block_to_cache;
};

}