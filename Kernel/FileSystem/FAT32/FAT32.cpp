#include "FAT32.h"

namespace kernel {

RefPtr<FileSystem> FAT32::create(StorageDevice& associated_device, LBARange lba_range)
{
    auto* fat32 = new FAT32(associated_device, lba_range);
    if (fat32->try_initialize())
        return { static_cast<FileSystem*>(fat32) };

    return {};
}

FAT32::FAT32(StorageDevice& associated_device, LBARange lba_range)
    : FileSystem(associated_device, lba_range)
{
}

bool FAT32::try_initialize()
{
    static constexpr size_t ebpb_offset = 0x0B;

    auto meta_region = MemoryManager::the().allocate_dma_buffer("FAT32 Meta"_sv, Page::size);
    auto info = associated_device().query_info();
    auto lba_count = Page::size / info.logical_block_size;
    ASSERT(lba_count != 0);

    auto request = StorageDevice::AsyncRequest::make_read(meta_region->virtual_range().begin(),
        { lba_range().begin(), lba_count });
    associated_device().submit_request(request);
    request.wait();

    copy_memory(meta_region->virtual_range().begin().as_pointer<u8>() + ebpb_offset, &m_ebpb, EBPB::size);

    if (m_ebpb.bytes_per_sector != info.logical_block_size) {
        log("FAT32") << "EBPB contains incorrect logical block size, expected "
                     << info.logical_block_size << " got " << m_ebpb.bytes_per_sector;
        return false;
    }

    static constexpr u8 ebpb_signature = 0x29;
    static constexpr StringView fat32_filesystem = "FAT32   "_sv;

    if (m_ebpb.signature != ebpb_signature) {
        log("FAT32") << "bad EBPB signature, expected " << format::as_hex
                     << ebpb_signature << " got " << m_ebpb.signature;
        return false;
    }

    if (m_ebpb.filesystem_type != fat32_filesystem) {
        log("FAT32") << "unexpected filesystem type " << StringView(m_ebpb.filesystem_type, 8);
        return false;
    }

    log("FAT32") << "Successfully parsed EBPB: " << m_ebpb.fat_count << " FATs, "
                 << m_ebpb.sectors_per_cluster << " sector(s) per cluster, "
                 << m_ebpb.sectors_per_fat << " sectors per FAT";

    auto fat_range = lba_range();
    fat_range.advance_begin_by(m_ebpb.reserved_sectors);
    fat_range.set_length(m_ebpb.sectors_per_fat);

    auto data_range = lba_range();
    data_range.advance_begin_by(m_ebpb.reserved_sectors);
    data_range.advance_begin_by(m_ebpb.sectors_per_fat * m_ebpb.fat_count);
    m_cluster_count = data_range.length() / m_ebpb.sectors_per_cluster;

    auto total_ram = MemoryManager::the().physical_stats().total_bytes;
    auto one_percent_of_ram = total_ram / 100;
    static constexpr u64 max_fat_cache_size = 5 * MB;
    static constexpr u64 max_data_cache_size = 256 * MB;

    auto fat_cache_size = min<u64>(one_percent_of_ram, max_fat_cache_size);
    fat_cache_size = min<u64>(fat_cache_size, m_ebpb.sectors_per_fat * m_ebpb.bytes_per_sector);
    log("FAT32") << "FAT cache size is ~" << fat_cache_size / KB << " KB";
    fat_cache_size /= sizeof(u32);

    auto bytes_per_cluster = m_ebpb.sectors_per_cluster * m_ebpb.bytes_per_sector;
    auto data_cache_size = min<u64>(one_percent_of_ram * 2, max_data_cache_size);
    data_cache_size = min<u64>(data_cache_size, m_cluster_count * bytes_per_cluster);
    log("FAT32") << "data cache size is ~" << data_cache_size / KB << " KB";
    data_cache_size /= bytes_per_cluster;

    m_fat_cache = new DiskCache(associated_device(), fat_range, sizeof(u32), fat_cache_size);
    m_data_cache = new DiskCache(associated_device(), data_range, bytes_per_cluster, data_cache_size);

    if (m_ebpb.fs_information_sector && m_ebpb.fs_information_sector != 0xFFFF) {
        log("FAT32") << "FSINFO at offset " << m_ebpb.fs_information_sector;

        auto& fsinfo = m_fsinfo.emplace();

        // We already have it read
        if (m_ebpb.fs_information_sector < lba_count) {
            auto offset = m_ebpb.fs_information_sector * m_ebpb.bytes_per_sector;
            copy_memory(Address(meta_region->virtual_range().begin() + offset).as_pointer<void>(), &fsinfo, FSINFO::size);
        } else {
            auto fsinfo_region = MemoryManager::the().allocate_dma_buffer("FSINFO"_sv, FSINFO::size);
            auto request = StorageDevice::AsyncRequest::make_read(
                fsinfo_region->virtual_range().begin(),
                { lba_range().begin() + m_ebpb.fs_information_sector, 1 });
            associated_device().submit_request(request);
            request.wait();
            copy_memory(fsinfo_region->virtual_range().begin().as_pointer<void>(), &fsinfo, FSINFO::size);
            MemoryManager::the().free_virtual_region(*fsinfo_region);
        }

        if (fsinfo.free_cluster_count < m_cluster_count && (fsinfo.last_allocater_cluster > 1 && fsinfo.last_allocater_cluster < m_cluster_count)) {
            log("FAT32") << "FSINFO seems to be valid: last allocated cluster " << fsinfo.last_allocater_cluster
                         << ", free clusters " << fsinfo.free_cluster_count;
        } else {
            log("FAT32") << "FSINFO contains bogus values, ignored";
            m_fsinfo.reset();
        }
    } else {
        log("FAT32") << "no FSINFO sector for this volume";
        // count free clusters manually
    }

    log("FAT32") << "total clusters " << m_cluster_count << ", free clusters " << (m_fsinfo ? m_fsinfo->free_cluster_count : 0);

    MemoryManager::the().free_virtual_region(*meta_region);

    return true;
}

FAT32::File::File(StringView name, FileSystem& filesystem, Attributes attributes, u32 first_cluster)
    : BaseFile(name, filesystem, attributes)
    , m_first_cluster(first_cluster)
{
}

Pair<ErrorCode, FAT32::BaseFile*> FAT32::open(StringView)
{
    return { ErrorCode::UNSUPPORTED, nullptr };
}

ErrorCode FAT32::close(BaseFile&)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::remove(StringView)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::create(StringView, File::Attributes)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::move(StringView, StringView)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::copy(StringView, StringView)
{
    return ErrorCode::UNSUPPORTED;
}

}