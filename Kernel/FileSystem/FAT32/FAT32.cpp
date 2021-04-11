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

FAT32::Directory::Directory(FileSystem& filesystem, u32 first_cluster)
    : BaseDirectory(filesystem)
    , m_first_cluster(first_cluster)
    , m_current_cluster(first_cluster)
{
}

u32 FAT32::pure_cluster_value(u32 value)
{
    ASSERT(value >= reserved_cluster_count);

    return value - reserved_cluster_count;
}

bool FAT32::Directory::fetch_next(void* into)
{
    if (m_exhausted)
        return false;

    auto& fs = fs_as_fat32();

    if (m_offset_within_cluster == fs.bytes_per_cluster()) {
        m_current_cluster = fs.fat_entry_at(m_current_cluster);

        auto type = fs.entry_type_of_fat_value(m_current_cluster);

        if (type == FATEntryType::END_OF_CHAIN) {
            m_exhausted = true;
            return false;
        } else if (type != FATEntryType::LINK) {
            String error_string;
            error_string << "FAT32: cluster chain contains invalid cluster value of " << m_current_cluster;
            runtime::panic(error_string.c_string());
        }

        m_offset_within_cluster = 0;
    }

    fs.m_data_cache->read_one(pure_cluster_value(m_current_cluster), m_offset_within_cluster, DirectoryEntry::size_in_bytes, into);
    m_offset_within_cluster += DirectoryEntry::size_in_bytes;

    return true;
}

FAT32::Directory::Entry FAT32::Directory::next()
{
    Entry out {};

    if (m_exhausted)
        return out;

    DirectoryEntry normal_entry;

    auto process_normal_entry = [](DirectoryEntry& in, Entry& out, bool is_small)
    {
        if (in.is_lowercase_name())
            String::to_lower(in.filename);
        if (in.is_lowercase_extension())
            String::to_lower(in.extension);

        auto name = StringView::from_char_array(in.filename);
        auto ext = StringView::from_char_array(in.extension);

        auto* out_buffer = is_small ? out.small_name : out.name;

        auto name_length = name.find(" "_sv).value_or(name.size());
        copy_memory(name.data(), out_buffer, name_length);

        auto ext_length = ext.find(" "_sv).value_or(ext.size());

        if (ext_length) {
            out_buffer[name_length++] = '.';
            copy_memory(ext.data(), out_buffer + name_length, ext_length);
        }

        out.size = in.size;

        if (in.is_directory())
            out.attributes = File::Attributes::IS_DIRECTORY;
    };

    auto process_ucs2 = [](const u8* ucs2, size_t count, char*& out) -> size_t
    {
        for (size_t i = 0; i < (count * bytes_per_ucs2_char); i += bytes_per_ucs2_char) {
            u16 ucs2_char = ucs2[i] | (ucs2[i + 1] << 8);

            char ascii;

            if (ucs2_char == 0) {
                return (i / bytes_per_ucs2_char);
            } else if (ucs2_char > 127) {
                ascii = '?';
            } else {
                ascii = static_cast<char>(ucs2_char & 0xFF);
            }

            *(out++) = ascii;
        }

        return count;
    };

    for (;;) {
        if (!fetch_next(&normal_entry))
            return out;

        if (normal_entry.is_deleted())
            continue;

        if (normal_entry.is_end_of_directory()) {
            m_exhausted = true;
            return out;
        }

        if (normal_entry.is_archive())
            continue;
        if (normal_entry.is_device())
            continue;

        auto is_long = normal_entry.is_long_name();

        if (!is_long && normal_entry.is_volume_label())
            continue;

        if (!is_long) {
            process_normal_entry(normal_entry, out, false);
            return out;
        }

        auto long_entry = LongNameDirectoryEntry::from_normal(normal_entry);
        auto initial_sequence_number = long_entry.extract_sequence_number();

        static constexpr u8 max_sequence_number = 20;
        ASSERT(initial_sequence_number <= max_sequence_number);

        auto sequence_number = initial_sequence_number;
        ASSERT(long_entry.is_last_logical());

        auto* cur_name_buffer_ptr = out.name + File::max_name_length;
        size_t chars_written = 0;

        for (;;) {
            cur_name_buffer_ptr -= LongNameDirectoryEntry::characters_per_entry;
            auto* name_ptr = cur_name_buffer_ptr;

            auto chars = process_ucs2(long_entry.name_1, long_entry.name_1_characters, name_ptr);
            chars_written += chars;

            if (chars == long_entry.name_1_characters) {
                chars = process_ucs2(long_entry.name_2, long_entry.name_2_characters, name_ptr);
                chars_written += chars;
            }

            if (chars == long_entry.name_2_characters) {
                chars = process_ucs2(long_entry.name_3, long_entry.name_3_characters, name_ptr);
                chars_written += chars;
            }

            if (sequence_number == 1) {
                auto did_fetch = fetch_next(&normal_entry);
                ASSERT(did_fetch);
                break;
            }

            auto did_fetch = fetch_next(&long_entry);
            ASSERT(did_fetch);
            --sequence_number;
        }

        ASSERT(chars_written <= File::max_name_length);

        if (cur_name_buffer_ptr != out.name)
            move_memory(cur_name_buffer_ptr, out.name, chars_written);

        out.name[chars_written] = '\0';

        process_normal_entry(normal_entry, out, true);
        return out;
    }
}

FAT32::FATEntryType FAT32::entry_type_of_fat_value(u32 value)
{
    if (value == 0)
        return FATEntryType::FREE;
    if (value == 1)
        return FATEntryType::RESERVED;

    if (value >= end_of_chain_min_cluster) {
        if (value != m_end_of_chain) {
            FAT32_WARN << "EOC value doesn't match FAT[1], " << format::as_hex
            << value << " vs " << m_end_of_chain;
        }

        return FATEntryType::END_OF_CHAIN;
    }

    if (value == bad_cluster)
        return FATEntryType::BAD;

    return FATEntryType::LINK;
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
        FAT32_WARN << "unexpected filesystem type " << StringView::from_char_array(m_ebpb.filesystem_type);
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

    m_bytes_per_cluster = m_ebpb.sectors_per_cluster * m_ebpb.bytes_per_sector;

    auto data_cache_size = min<u64>(one_percent_of_ram * 2, max_data_cache_size);
    data_cache_size = min<u64>(data_cache_size, m_cluster_count * m_bytes_per_cluster);
    FAT32_LOG << "data cache size is ~" << data_cache_size / KB << " KB";
    data_cache_size = Page::round_down(data_cache_size);
    data_cache_size /= m_bytes_per_cluster;

    m_fat_cache = new DiskCache(associated_device(), fat_range, sizeof(u32), fat_cache_size);
    m_data_cache = new DiskCache(associated_device(), data_range, m_bytes_per_cluster, data_cache_size);

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
            auto fsinfo_request = StorageDevice::AsyncRequest::make_read(
                fsinfo_region->virtual_range().begin(),
                { lba_range().begin() + m_ebpb.fs_information_sector, 1 });
            associated_device().submit_request(fsinfo_request);
            fsinfo_request.wait();
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
    m_end_of_chain = fat_entry_at(end_of_chain_indicator_fat_entry);
    FAT32_LOG << "end of chain is " << format::as_hex << m_end_of_chain;

    MemoryManager::the().free_virtual_region(*meta_region);

    return true;
}

void FAT32::calculate_capacity()
{
    auto last_fat_entry = (m_cluster_count + 2) - 1;

    for (; last_fat_entry > 1; --last_fat_entry) {
        auto cluster = fat_entry_at(last_fat_entry);

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

u32 FAT32::fat_entry_at(u32 index)
{
    u32 value = 0;
    m_fat_cache->read_one(index, 0, sizeof(u32), &value);

    return value;
}

void FAT32::set_fat_entry_at(u32 index, u32 value)
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