#include "DiskCache.h"
#include "Memory/MemoryManager.h"

namespace kernel {

DiskCache::DiskCache(StorageDevice& device, LBARange filesystem_lba_range, size_t filesystem_block_size, size_t block_capacity)
    : m_device(device)
    , m_fs_lba_range(filesystem_lba_range)
    , m_fs_block_size(filesystem_block_size)
    , m_capacity(block_capacity)
{
    auto info = device.query_info();

    ASSERT(info.logical_block_size == 512 || info.logical_block_size == 4096);

    m_logical_block_size = info.logical_block_size;

    // Filesystem is entirely in RAM, no need to cache IO.
    if (info.medium_type == StorageDevice::Info::MediumType::RAM) {
        m_io_size = 0;
    }
    if (m_fs_block_size >= Page::size) {
        ASSERT_PAGE_ALIGNED(m_fs_block_size);
        m_io_size = m_fs_block_size;
    } else { // minimum IO size is 4K
        ASSERT((m_io_size % m_fs_block_size) == 0);
        m_io_size = Page::size;
    }

    m_fs_blocks_per_io = m_io_size / m_fs_block_size;

    auto region = MemoryManager::the().allocate_kernel_private_anywhere("DiskCache", m_fs_block_size * m_capacity);
    m_region = static_cast<PrivateVirtualRegion*>(region.get());

    if (!m_fs_lba_range.begin() % 8 && info.logical_block_size == 512) {
        warning() << "DiskCache: partition starts at an unaligned logical block "
                  << filesystem_lba_range.begin() << ", expect poor performance";
    }
}

u64 DiskCache::block_to_first_lba(u64 block_index) const
{
    u64 offset = block_index * m_fs_block_size;
    offset /= m_logical_block_size;

    return offset;
}

u64 DiskCache::block_to_first_lba_within_fs(u64 block_index) const
{
    return m_fs_lba_range.begin() + block_to_first_lba(block_index);
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
    block_index &= ~(m_fs_blocks_per_io - 1);
    auto offset = (original_index - block_index) * m_fs_block_size;

    auto block = m_block_to_cache.find(block_index);
    if (block != m_block_to_cache.end()) {
        auto* cached_entry = block->second();
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

    if (block_to_evict.is_dirty()) {
        auto range = block_to_lba_range(block_to_evict.first_block);
        auto request = StorageDevice::AsyncRequest::make_write(block_to_evict.virtual_address(), range);
        m_device.submit_request(request);
        request.wait();
        block_to_evict.mark_clean();
    }

    m_block_to_cache.remove(block_to_evict.first_block);
    block_to_evict.first_block = 0;

    return &block_to_evict;
}

void DiskCache::read_one(u64 block_index, size_t offset, size_t bytes, void* buffer)
{
    if (m_io_size == no_caching_required) {
        auto full_offset = offset + block_index * m_fs_block_size;
        auto req = StorageDevice::RamdiskRequest::make_read(buffer, full_offset, bytes);
        m_device.submit_request(req);
        return;
    }

    auto block_and_offset = cached_block(block_index);

    auto begin = block_and_offset.first()->virtual_address();
    begin += block_and_offset.second();
    begin += offset;

    copy_memory(begin.as_pointer<void>(), buffer, bytes);
}

void DiskCache::write_one(u64 block_index, size_t offset, size_t bytes, void* buffer)
{
    if (m_io_size == no_caching_required) {
        auto full_offset = offset + block_index * m_fs_block_size;
        auto req = StorageDevice::RamdiskRequest::make_read(buffer, full_offset, bytes);
        m_device.submit_request(req);
        return;
    }

    auto block_and_offset = cached_block(block_index);

    auto begin = block_and_offset.first()->virtual_address();
    begin += block_and_offset.second();
    begin += offset;

    copy_memory(buffer, begin.as_pointer<void>(), bytes);
    block_and_offset.first()->mark_dirty();
}

}