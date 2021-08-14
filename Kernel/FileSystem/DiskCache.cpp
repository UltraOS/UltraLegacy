#include "DiskCache.h"
#include "Memory/MemoryManager.h"
#include "Memory/SafeOperations.h"

#define DC_DEBUG_MODE

#define DC_LOG log("DiskCache")
#define DC_WARN warning("DiskCache")

#ifdef DC_DEBUG_MODE
#define DC_DEBUG log("DiskCache")
#else
#define DC_DEBUG DummyLogger()
#endif

namespace kernel {

DiskCache::DiskCache(StorageDevice& device, LBARange filesystem_lba_range, size_t filesystem_block_size, size_t block_capacity)
    : m_device(device)
    , m_fs_lba_range(filesystem_lba_range)
    , m_fs_block_size(filesystem_block_size)
    , m_capacity(block_capacity)
{
    ASSERT(block_capacity != 0);

    auto info = device.query_info();

    ASSERT(info.logical_block_size == 512 || info.logical_block_size == 4096);

    m_logical_block_size = info.logical_block_size;

    // Filesystem is entirely in RAM, no need to cache IO.
    if (info.medium_type == StorageDevice::Info::MediumType::RAM) {
        m_io_size = 0;
        return;
    }

    if (m_fs_block_size >= Page::size) {
        ASSERT_PAGE_ALIGNED(m_fs_block_size);
        m_io_size = m_fs_block_size;
    } else { // minimum IO size is 4K
        ASSERT((m_io_size % m_fs_block_size) == 0);
        m_io_size = Page::size;
    }

    m_fs_blocks_per_io = m_io_size / m_fs_block_size;

    auto region = MemoryManager::the().allocate_kernel_private_anywhere("DiskCache", m_fs_block_size * block_capacity);
    m_region = static_cast<PrivateVirtualRegion*>(region.get());

    // capacity is measured in pages, not in FS blocks, unless FS block is over 4K
    m_capacity /= m_fs_blocks_per_io;
    ASSERT(m_capacity != 0);

    DC_DEBUG << "cache block capacity is " << m_capacity << ", a cache block is " << m_fs_blocks_per_io << " FS blocks";

    if (!m_fs_lba_range.begin() % 8 && info.logical_block_size == 512) {
        DC_WARN << "partition starts at an unaligned logical block "
                << filesystem_lba_range.begin() << ", expect poor performance";
    }
}

u64 DiskCache::block_to_first_lba(u64 block_index) const
{
    u64 offset = block_index * m_fs_block_size;
    offset /= m_logical_block_size;

    return offset;
}

LBARange DiskCache::block_to_lba_range(u64 block_index) const
{
    auto lba = block_to_first_lba(block_index);
    auto logical_blocks_per_cache_block = m_io_size / m_logical_block_size;

    // FS block size is less than 4K
    if (m_fs_block_size < m_io_size && m_logical_block_size == 512)
        lba &= ~0b111; // align to 8

    return { m_fs_lba_range.begin() + lba, logical_blocks_per_cache_block };
}

u64 DiskCache::block_index_to_cached_index(u64 block_index)
{
    return block_index & ~(m_fs_blocks_per_io - 1);
}

Address DiskCache::allocate_next_block_buffer()
{
    auto next_address = m_region->virtual_range().begin() + m_offset_within_region;
    m_offset_within_region += m_io_size;

    m_region->preallocate_specific({ next_address, m_io_size });
    return next_address;
}

Pair<DiskCache::CachedBlock*, size_t> DiskCache::cached_block(u64 block_index)
{
    auto original_index = block_index;
    block_index = block_index_to_cached_index(block_index);
    auto offset = (original_index - block_index) * m_fs_block_size;

    auto block = m_block_to_cache.find(block_index);
    if (block != m_block_to_cache.end()) {
        auto* cached_entry = block->second;
        cached_entry->pop_off();
        m_cached_blocks.insert_front(*cached_entry);
        return { cached_entry, offset };
    }

    CachedBlock* new_cached_block = nullptr;
    if (m_cached_blocks.size() == m_capacity) {
        new_cached_block = evict_one();
    } else {
        new_cached_block = new CachedBlock();
        new_cached_block->virtual_address_and_dirty_bit = allocate_next_block_buffer();
    }

    auto lba_range = block_to_lba_range(block_index);
    ASSERT(m_fs_lba_range.contains(lba_range));

    auto request = StorageDevice::AsyncRequest::make_read(new_cached_block->virtual_address(), lba_range);
    m_device.submit_request(request);
    request.wait();

    new_cached_block->first_block = block_index;
    m_cached_blocks.insert_front(*new_cached_block);
    m_block_to_cache[block_index] = new_cached_block;

    return { new_cached_block, offset };
}

DiskCache::CachedBlock* DiskCache::evict_one()
{
    auto& block_to_evict = m_cached_blocks.pop_back();

    DC_DEBUG << "evicting cached block " << block_to_evict.first_block;

    if (block_to_evict.is_dirty())
        flush_block(block_to_evict);

    m_block_to_cache.remove(block_to_evict.first_block);
    block_to_evict.first_block = 0;

    return &block_to_evict;
}

ErrorCode DiskCache::read_one(u64 block_index, size_t offset, size_t bytes, void* buffer)
{
    ASSERT((offset + bytes) <= m_fs_block_size);

    if (m_io_size == no_caching_required) {
        auto full_offset = offset + block_index * m_fs_block_size;
        auto req = StorageDevice::RamdiskRequest::make_read(buffer, full_offset, bytes);
        m_device.submit_request(req);
        return req.result();
    }

    auto block_and_offset = cached_block(block_index);

    auto begin = block_and_offset.first->virtual_address();
    begin += block_and_offset.second;
    begin += offset;

    if (!safe_copy_memory(begin.as_pointer<void>(), buffer, bytes))
        return ErrorCode::MEMORY_ACCESS_VIOLATION;

    return ErrorCode::NO_ERROR;
}

ErrorCode DiskCache::write_one(u64 block_index, size_t offset, size_t bytes, const void* buffer)
{
    ASSERT((offset + bytes) <= m_fs_block_size);

    if (m_io_size == no_caching_required) {
        auto full_offset = offset + block_index * m_fs_block_size;
        auto req = StorageDevice::RamdiskRequest::make_read(buffer, full_offset, bytes);
        m_device.submit_request(req);
        return req.result();
    }

    auto block_and_offset = cached_block(block_index);

    auto begin = block_and_offset.first->virtual_address();
    begin += block_and_offset.second;
    begin += offset;

    block_and_offset.first->mark_dirty();
    if (!safe_copy_memory(buffer, begin.as_pointer<void>(), bytes))
        return ErrorCode::MEMORY_ACCESS_VIOLATION;

    return ErrorCode::NO_ERROR;
}

void DiskCache::zero_fill_one(u64 block_index)
{
    if (m_io_size == no_caching_required) {
        auto full_offset = block_index * m_fs_block_size;
        auto* zeroed_buf = new u8[m_fs_block_size] {};
        auto req = StorageDevice::RamdiskRequest::make_write(zeroed_buf, full_offset, m_fs_block_size);
        m_device.submit_request(req);
        delete[] zeroed_buf;
        return;
    }

    auto block_and_offset = cached_block(block_index);
    auto begin = block_and_offset.first->virtual_address();
    begin += block_and_offset.second;
    zero_memory(begin.as_pointer<void>(), m_fs_block_size);
    block_and_offset.first->mark_dirty();
}

void DiskCache::flush_block(CachedBlock& block)
{
    if (!block.is_dirty())
        return;

    DC_DEBUG << "flushing block " << block.first_block;

    auto range = block_to_lba_range(block.first_block);
    ASSERT(m_fs_lba_range.contains(range));

    auto request = StorageDevice::AsyncRequest::make_write(block.virtual_address(), range);
    m_device.submit_request(request);
    request.wait();
    block.mark_clean();
}

void DiskCache::flush_all()
{
    if (m_io_size == no_caching_required)
        return;

    size_t flushed_count = 0;

    for (auto& block : m_cached_blocks) {
        if (!block.is_dirty())
            continue;

        flush_block(block);
        ++flushed_count;
    }

    if (flushed_count)
        DC_LOG << "flushed " << flushed_count << " blocks";
}

void DiskCache::flush_specific(u64 block_index)
{
    if (m_io_size == no_caching_required)
        return;

    auto aligned_index = block_index_to_cached_index(block_index);

    auto cached_block = m_block_to_cache.find(aligned_index);
    if (cached_block == m_block_to_cache.end()) {
        DC_WARN << "was asked to flush uncached block " << block_index;
        return;
    }

    flush_block(*cached_block->second);
}

}
