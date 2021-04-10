#include "FAT32.h"

#define FAT32_LOG log("FAT32")
#define FAT32_WARN warning("FAT32")

#ifdef FAT32_DEBUG_MODE
#define FAT32_DEBUG log("FAT32")
#else
#define FAT32_DEBUG DummyLogger()
#endif

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
        FAT32_WARN << "EBPB contains incorrect logical block size, expected "
                   << info.logical_block_size << " got " << m_ebpb.bytes_per_sector;
        return false;
    }

    static constexpr u8 ebpb_signature = 0x29;
    static constexpr StringView fat32_filesystem = "FAT32   "_sv;

    if (m_ebpb.signature != ebpb_signature) {
        FAT32_WARN << "bad EBPB signature, expected " << format::as_hex
                   << ebpb_signature << " got " << m_ebpb.signature;
        return false;
    }

    if (m_ebpb.filesystem_type != fat32_filesystem) {
        FAT32_WARN << "unexpected filesystem type " << StringView(m_ebpb.filesystem_type, 8);
        return false;
    }

    FAT32_LOG << "Successfully parsed EBPB: " << m_ebpb.fat_count << " FATs, "
                 << m_ebpb.sectors_per_cluster << " sector(s) per cluster, "
                 << m_ebpb.sectors_per_fat << " sectors per FAT";

    auto fat_range = lba_range();
    fat_range.advance_begin_by(m_ebpb.reserved_sectors);
    fat_range.set_length(m_ebpb.sectors_per_fat);

    auto data_range = lba_range();
    data_range.advance_begin_by(m_ebpb.reserved_sectors);
    data_range.advance_begin_by(m_ebpb.sectors_per_fat * m_ebpb.fat_count);
    m_cluster_count = data_range.length() / m_ebpb.sectors_per_cluster;

    static constexpr auto min_cluster_count_for_fat32 = 65525;
    if (m_cluster_count < min_cluster_count_for_fat32) {
        FAT32_LOG << "cluster count is too low, expected at least "
                  << min_cluster_count_for_fat32 << " got " << m_cluster_count;
        return false;
    }

    auto total_ram = MemoryManager::the().physical_stats().total_bytes;
    auto one_percent_of_ram = total_ram / 100;
    static constexpr u64 max_fat_cache_size = 5 * MB;
    static constexpr u64 max_data_cache_size = 256 * MB;

    auto fat_cache_size = min<u64>(one_percent_of_ram, max_fat_cache_size);
    fat_cache_size = min<u64>(fat_cache_size, m_ebpb.sectors_per_fat * m_ebpb.bytes_per_sector);
    FAT32_LOG << "FAT cache size is ~" << fat_cache_size / KB << " KB";
    fat_cache_size = Page::round_down(fat_cache_size);
    fat_cache_size /= sizeof(u32);

    auto bytes_per_cluster = m_ebpb.sectors_per_cluster * m_ebpb.bytes_per_sector;
    auto data_cache_size = min<u64>(one_percent_of_ram * 2, max_data_cache_size);
    data_cache_size = min<u64>(data_cache_size, m_cluster_count * bytes_per_cluster);
    FAT32_LOG << "data cache size is ~" << data_cache_size / KB << " KB";
    data_cache_size = Page::round_down(data_cache_size);
    data_cache_size /= bytes_per_cluster;

    m_fat_cache = new DiskCache(associated_device(), fat_range, sizeof(u32), fat_cache_size);
    m_data_cache = new DiskCache(associated_device(), data_range, bytes_per_cluster, data_cache_size);

    bool should_manually_calculate_capacity = false;

    if (m_ebpb.fs_information_sector && m_ebpb.fs_information_sector != 0xFFFF) {
        FAT32_DEBUG << "FSINFO at offset " << m_ebpb.fs_information_sector;

        FSINFO fsinfo {};

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

        should_manually_calculate_capacity = !parse_fsinfo(fsinfo);
    } else {
        FAT32_LOG << "no FSINFO sector for this volume";
        should_manually_calculate_capacity = true;
    }

    if (should_manually_calculate_capacity)
        calculate_capacity();

    FAT32_LOG << "total cluster count " << m_cluster_count << ", free clusters " << m_free_clusters;

    static constexpr u32 end_of_chain_indicator_fat_entry = 1;
    m_end_of_chain = get_fat_entry(end_of_chain_indicator_fat_entry);
    FAT32_LOG << "end of chain is " << format::as_hex << m_end_of_chain;

    MemoryManager::the().free_virtual_region(*meta_region);

    return true;
}

void FAT32::calculate_capacity()
{
    auto last_fat_entry = (m_cluster_count + 2) - 1;

    for (; last_fat_entry > 1; --last_fat_entry) {
        auto cluster = get_fat_entry(last_fat_entry);

        if (cluster == free_cluster) {
            m_free_clusters++;
            m_last_free_cluster = cluster;
        }
    }
}

bool FAT32::parse_fsinfo(FSINFO& fsinfo)
{
    static constexpr StringView fsinfo_signature_1 = "RRaA"_sv;
    static constexpr StringView fsinfo_signature_2 = "rrAa"_sv;
    static constexpr uint8_t fsinfo_signature_3[] = { 0x00, 0x00, 0x55, 0xAA };

    auto log_signature_mismatch = [] (size_t index)
    {
        FAT32_WARN << "FSINFO signature " << index << " is invalid";
    };

    if (fsinfo.signature_1 != fsinfo_signature_1) {
        log_signature_mismatch(1);
        return false;
    }

    if (fsinfo.signature_2 != fsinfo_signature_2) {
        log_signature_mismatch(2);
        return false;
    }

    if (!compare_memory(fsinfo.signature_3, fsinfo_signature_3, 4)) {
        log_signature_mismatch(3);
        return false;
    }

    if (fsinfo.free_cluster_count == 0xFFFFFFFF || fsinfo.free_cluster_count > m_cluster_count) {
        FAT32_WARN << "FSINFO contains invalid free cluster count of " << fsinfo.free_cluster_count;
        return false;
    }

    if (fsinfo.last_allocated_cluster < 2 || fsinfo.last_allocated_cluster >= m_cluster_count) {
        FAT32_WARN << "FSINFO contains invalid last allocated cluster " << fsinfo.last_allocated_cluster;
        return false;
    }

    FAT32_LOG << "FSINFO seems to be valid: last allocated cluster " << fsinfo.last_allocated_cluster
              << ", free clusters " << fsinfo.free_cluster_count;

    m_free_clusters = fsinfo.free_cluster_count;
    m_last_free_cluster = fsinfo.last_allocated_cluster + 1;

    return true;
}

u32 FAT32::get_fat_entry(u32 index)
{
    u32 value = 0;
    m_fat_cache->read_one(index, 0, sizeof(u32), &value);

    return value;
}

void FAT32::put_fat_entry(u32 index, u32 value)
{
    m_fat_cache->write_one(index, 0, sizeof(u32), &value);
}

FAT32::File::File(StringView name, FileSystem& filesystem, Attributes attributes, u32 first_cluster)
    : BaseFile(name, filesystem, attributes)
    , m_first_cluster(first_cluster)
{
}

Pair<ErrorCode, FAT32::BaseDirectory*> FAT32::open_directory(StringView)
{
    return { ErrorCode::UNSUPPORTED, nullptr };
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