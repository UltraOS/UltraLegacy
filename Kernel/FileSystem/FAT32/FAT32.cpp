#include "FAT32.h"
#include "FileSystem/Utilities.h"
#include "Memory/TypedMapping.h"
#include "Utilities.h"

#define FAT32_LOG log("FAT32")
#define FAT32_WARN warning("FAT32")

#define FAT32_DEBUG_MODE
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

    delete fat32;
    return {};
}

FAT32::FAT32(StorageDevice& associated_device, LBARange lba_range)
    : FileSystem(associated_device, lba_range)
{
}

FAT32::Directory::Directory(FileSystem& filesystem, File& directory_file, bool owns_file)
    : BaseDirectory(filesystem, directory_file)
    , m_current_cluster(first_cluster())
    , m_owns_file(owns_file)
{
    ASSERT(file().is_directory());

    if (m_current_cluster == free_cluster)
        m_exhausted = true;
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

    if (m_owns_file)
        file().lock().lock();

    if (m_offset_within_cluster == fs.bytes_per_cluster()) {
        m_current_cluster = fs.locked_fat_entry_at(m_current_cluster);

        auto type = fs.entry_type_of_fat_value(m_current_cluster);

        if (type == FATEntryType::END_OF_CHAIN) {
            m_exhausted = true;

            if (m_owns_file)
                file().lock().unlock();

            return false;
        } else if (type != FATEntryType::LINK) {
            String error_string;
            error_string << "FAT32: cluster chain contains invalid cluster value of " << m_current_cluster;
            runtime::panic(error_string.c_string());
        }

        m_offset_within_cluster = 0;
    }

    fs.locked_read(pure_cluster_value(m_current_cluster), m_offset_within_cluster, DirectoryEntry::size_in_bytes, into);

    m_offset_within_cluster += DirectoryEntry::size_in_bytes;

    if (m_owns_file)
        file().lock().unlock();

    return true;
}

FAT32::Directory::Slot FAT32::Directory::allocate_entries(size_t count)
{
    auto& fs = fs_as_fat32();

    FAT32_DEBUG << "allocating " << count << " directory slots for directory at " << first_cluster();

    u32 allocated_clusters[3] {};
    u32 last_allocated_cluster = 0;
    u32 last_allocated_index = 0;
    u32 initial_offset = 0;

    u32 current_cluster = first_cluster();
    u32 current_offset = 0;
    u32 contiguous_empty = 0;

    for (;;) {
        u8 first_byte = 0;
        fs.locked_read(pure_cluster_value(current_cluster), current_offset, 1, &first_byte);

        if (first_byte == DirectoryEntry::end_of_directory_mark || first_byte == DirectoryEntry::deleted_mark) {
            contiguous_empty++;

            FAT32_DEBUG << "entry at " << current_cluster << ", offset " << current_offset
                        << " is empty, empty count so far is " << contiguous_empty;

            if (last_allocated_cluster != current_cluster) {
                if (initial_offset == 0)
                    initial_offset = current_offset;

                allocated_clusters[last_allocated_index++] = current_cluster;
                last_allocated_cluster = current_cluster;
            }

            if (contiguous_empty == count)
                return { initial_offset, allocated_clusters };
        } else {
            FAT32_DEBUG << "entry at " << current_cluster << ", offset " << current_offset
                        << " is not empty, resetting contiguous count of " << contiguous_empty;

            zero_memory(allocated_clusters, 3 * sizeof(u32));
            last_allocated_cluster = 0;
            last_allocated_index = 0;
            initial_offset = 0;
            contiguous_empty = 0;
        }

        current_offset += DirectoryEntry::size_in_bytes;

        if (current_offset == fs.bytes_per_cluster()) {
            current_cluster = fs.fat_entry_at(current_cluster);

            if (fs.entry_type_of_fat_value(current_cluster) == FATEntryType::END_OF_CHAIN)
                break;
        }
    }

    FAT32_DEBUG << "couldn't find exactly " << count << " contiguous free slots, found "
                << contiguous_empty << " at the end";

    auto extra_to_allocate = count - contiguous_empty;
    auto extra_bytes = extra_to_allocate * DirectoryEntry::size_in_bytes;
    auto clusters_to_allocate = ceiling_divide<size_t>(extra_bytes, fs.bytes_per_cluster());

    FAT32_DEBUG << "allocating extra " << extra_to_allocate << " directory entries, aka "
                << clusters_to_allocate << " cluster(s)";

    auto chain = fs.allocate_cluster_chain(clusters_to_allocate, file_as_fat32().compute_last_cluster());

    // We have to zero the last cluster in chain because entry count is unaligned to size
    if (extra_bytes != (clusters_to_allocate * fs.bytes_per_cluster())) {
        FAT32_DEBUG << "extra entries are unaligned to cluster size, zero-filling cluster "
                    << chain.last();

        fs.locked_zero_fill(pure_cluster_value(chain.last()), 1);
    }

    for (auto cluster : chain)
        allocated_clusters[last_allocated_index++] = cluster;

    return { initial_offset, allocated_clusters };
}

void FAT32::Directory::write_one(void* directory_entry, Slot& slot)
{
    auto& fs = fs_as_fat32();

    auto coords = slot.next_entry(fs.bytes_per_cluster());

    fs.locked_write(pure_cluster_value(coords.cluster), coords.offset_within_cluster, DirectoryEntry::size_in_bytes, directory_entry);
}

FAT32::Directory::Entry FAT32::Directory::next()
{
    return static_cast<Entry>(next_native());
}

FAT32::Directory::NativeEntry FAT32::Directory::next_native()
{
    NativeEntry out {};

    if (m_exhausted)
        return out;

    DirectoryEntry normal_entry {};

    auto process_normal_entry = [this](DirectoryEntry& in, NativeEntry& out, bool is_small) {
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
        out.first_data_cluster = (in.cluster_high << 16) | in.cluster_low;
        out.metadata_entry_cluster = m_current_cluster;
        out.metadata_entry_offset_within_cluster = m_offset_within_cluster - DirectoryEntry::size_in_bytes;

        if (in.is_directory())
            out.attributes = File::Attributes::IS_DIRECTORY;
    };

    auto process_ucs2 = [](const u8* ucs2, size_t count, char*& out) -> size_t {
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

        out.file_entry_cluster_1 = m_current_cluster;
        out.entry_offset_within_cluster = m_offset_within_cluster - DirectoryEntry::size_in_bytes;
        out.sequence_count = 0;

        auto is_long = normal_entry.is_long_name();

        if (!is_long && normal_entry.is_volume_label())
            continue;

        if (!is_long) {
            process_normal_entry(normal_entry, out, false);
            return out;
        }

        auto long_entry = LongNameDirectoryEntry::from_normal(normal_entry);
        auto initial_sequence_number = long_entry.extract_sequence_number();
        out.sequence_count = initial_sequence_number;
        ASSERT(initial_sequence_number <= max_sequence_number);

        auto sequence_number = initial_sequence_number;
        ASSERT(long_entry.is_last_logical());

        auto* cur_name_buffer_ptr = out.name + File::max_name_length;
        size_t chars_written = 0;

        auto previous_cluster = m_current_cluster;

        u32 checksum_array[max_sequence_number] {};

        for (;;) {
            // VFAT spans multiple clusters
            if (m_current_cluster != previous_cluster) {
                if (out.file_entry_cluster_2 == free_cluster)
                    out.file_entry_cluster_2 = m_current_cluster;
                else if (out.file_entry_cluster_3 == free_cluster)
                    out.file_entry_cluster_3 = m_current_cluster;
                else
                    ASSERT_NEVER_REACHED();

                previous_cluster = m_current_cluster;
            }

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

            checksum_array[sequence_number - 1] = long_entry.checksum;

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

        auto expected_checksum = generate_short_name_checksum(StringView(normal_entry.filename, short_name_length + short_extension_length));

        for (size_t i = 0; i < initial_sequence_number; ++i) {
            if (checksum_array[i] == expected_checksum)
                continue;

            FAT32_WARN << "VFAT name invalid checksum, expected "
                       << format::as_hex << expected_checksum << " got " << checksum_array[i];

            // TODO: ignore the entry?
        }

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

    FAT32_LOG << "total cluster count " << m_cluster_count << ", free clusters " << m_free_clusters.load();

    static constexpr u32 end_of_chain_indicator_fat_entry = 1;
    m_end_of_chain = locked_fat_entry_at(end_of_chain_indicator_fat_entry);
    FAT32_LOG << "end of chain is " << format::as_hex << m_end_of_chain;

    MemoryManager::the().free_virtual_region(*meta_region);

    m_root_directory = open_or_incref("root directory"_sv, File::Attributes::IS_DIRECTORY, {}, m_ebpb.root_dir_cluster, 0);

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

    auto log_signature_mismatch = [](size_t index) {
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

u32 FAT32::locked_fat_entry_at(u32 index)
{
    LOCK_GUARD(m_fat_cache_lock);
    return fat_entry_at(index);
}

void FAT32::locked_set_fat_entry_at(u32 index, u32 value)
{
    LOCK_GUARD(m_fat_cache_lock);
    set_fat_entry_at(index, value);
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

void FAT32::locked_read(u64 block_index, size_t offset, size_t bytes, void* buffer)
{
    LOCK_GUARD(m_data_lock);
    m_data_cache->read_one(block_index, offset, bytes, buffer);
}

void FAT32::locked_write(u64 block_index, size_t offset, size_t bytes, void* buffer)
{
    LOCK_GUARD(m_data_lock);
    m_data_cache->write_one(block_index, offset, bytes, buffer);
}

void FAT32::locked_zero_fill(u64 block_index, size_t count)
{
    LOCK_GUARD(m_data_lock);

    for (size_t i = 0; i < count; ++i)
        m_data_cache->zero_fill_one(block_index + i);
}

FAT32::File::File(StringView name, FileSystem& filesystem, Attributes attributes,
    const File::Identifier& identifier, u32 first_cluster, u32 size)
    : BaseFile(name, filesystem, attributes)
    , m_identifier(identifier)
    , m_first_cluster(first_cluster)
    , m_size(size)
{
}

size_t FAT32::File::read(void* buffer, size_t offset, size_t size)
{
    LOCK_GUARD(lock());

    if (offset >= this->size())
        return 0;

    auto& fs = fs_as_fat32();

    auto cluster_offset = offset / fs.bytes_per_cluster();
    auto offset_within_cluster = offset - (cluster_offset / fs.bytes_per_cluster());
    auto bytes_left_after_offset = this->size() - offset;

    size_t bytes_to_read = min(size, bytes_left_after_offset);
    size_t bytes_read = bytes_to_read;
    size_t clusters_to_read = ceiling_divide<size_t>(bytes_to_read, fs.bytes_per_cluster());

    u32 current_cluster = m_first_cluster;

    auto safe_next_of = [&fs](u32 cluster) {
        cluster = fs.locked_fat_entry_at(cluster);
        ASSERT(fs.entry_type_of_fat_value(cluster) == FATEntryType::LINK);
        return cluster;
    };

    while (cluster_offset--)
        current_cluster = safe_next_of(current_cluster);

    u8* byte_buffer = reinterpret_cast<u8*>(buffer);

    while (clusters_to_read--) {
        auto bytes_to_read_for_this_cluster = min<size_t>(bytes_to_read, fs.bytes_per_cluster());
        fs.locked_read(pure_cluster_value(current_cluster), offset_within_cluster, bytes_to_read_for_this_cluster, byte_buffer);
        byte_buffer += bytes_to_read_for_this_cluster;
        bytes_to_read -= bytes_to_read_for_this_cluster;

        if (clusters_to_read)
            current_cluster = safe_next_of(current_cluster);
    }

    return bytes_read;
}

size_t FAT32::File::write(void* buffer, size_t offset, size_t size)
{
    if (size == 0)
        return 0;

    auto& fs = fs_as_fat32();

    LOCK_GUARD(lock());

    auto offset_cluster_index = offset / fs.bytes_per_cluster();
    auto offset_within_cluster = offset - (offset_cluster_index * fs.bytes_per_cluster());
    auto file_cluster_count = ceiling_divide<size_t>(m_size, fs.bytes_per_cluster());
    bool write_starts_at_last_cluster = false;

    // We have to zero fill all the clusters before offset
    if (file_cluster_count <= offset_cluster_index) {
        write_starts_at_last_cluster = true;
        auto zero_filled_clusters = offset_cluster_index - file_cluster_count + 1;
        zero_filled_clusters += 1; // we allocate 1 extra cluster that will contain the written data

        // if write is greater than the cluster size or offset doesn't start at the
        // exact beginning of the cluster we have to zero fill it
        bool should_zero_fill_last_cluster = offset_within_cluster || (size < fs.bytes_per_cluster());

        FAT32_DEBUG << "write offset past last cluster for file " << name()
                    << " zero filled cluster count is " << (zero_filled_clusters - !should_zero_fill_last_cluster);

        auto chain = fs.allocate_cluster_chain(zero_filled_clusters, compute_last_cluster());

        for (size_t i = 0; i < (chain.size() - !should_zero_fill_last_cluster); ++i)
            fs.locked_zero_fill(pure_cluster_value(chain[i]), 1);

        if (!m_first_cluster) {
            FAT32_DEBUG << "file " << name() << " was empty, setting first cluster to be " << chain.first();
            set_first_cluster(chain.first());
        }

        FAT32_DEBUG << "new last cluster for file " << name() << " is " << chain.last();
        set_last_cluster(chain.last());
        file_cluster_count += zero_filled_clusters;
    }

    u32 first_cluster_to_write;

    if (write_starts_at_last_cluster) {
        first_cluster_to_write = compute_last_cluster();
    } else {
        if (offset_cluster_index)
            first_cluster_to_write = fs.nth_cluster_in_chain(first_cluster(), offset_cluster_index);
        else
            first_cluster_to_write = first_cluster();
    }

    auto bytes_to_write = size;
    auto clusters_to_write = size / fs.bytes_per_cluster();

    if ((offset_cluster_index + clusters_to_write + 1) > file_cluster_count) {
        auto clusters_to_allocate = (offset_cluster_index + clusters_to_write + 1) - file_cluster_count;
        auto chain = fs.allocate_cluster_chain(clusters_to_allocate, compute_last_cluster());

        FAT32_DEBUG << "write for file " << name() << " goes outside of last cluster, allocating "
                    << clusters_to_allocate << " extra";

        auto bytes_to_write_for_new_clusters = bytes_to_write - (fs.bytes_per_cluster() - offset_within_cluster);

        // write size is unaligned to cluster size so we zero fill the last cluster
        if (bytes_to_write_for_new_clusters % fs.bytes_per_cluster())
            fs.locked_zero_fill(chain.last(), 1);

        set_last_cluster(chain.last());
    }

    auto current_cluster = first_cluster_to_write;
    auto* byte_buff = reinterpret_cast<u8*>(buffer);

    while (bytes_to_write) {
        auto bytes_for_this_write = min(bytes_to_write, fs.bytes_per_cluster() - offset_within_cluster);
        fs.locked_write(pure_cluster_value(current_cluster), offset_cluster_index, bytes_for_this_write, byte_buff);

        byte_buff += bytes_for_this_write;
        bytes_to_write -= bytes_for_this_write;
        current_cluster = fs.fat_entry_at(current_cluster);
        ASSERT(fs.entry_type_of_fat_value(current_cluster) != FATEntryType::END_OF_CHAIN);
        offset_within_cluster = 0;
    }

    auto end_of_write = offset + size;
    if (end_of_write > m_size) {
        FAT32_DEBUG << "set new size for " << name() << " to be " << m_size;
        set_size(end_of_write);
        flush_meta_modifications();
    }

    return size;
}

void FAT32::File::flush_meta_modifications()
{
    if (!is_dirty())
        return;

    auto& fs = fs_as_fat32();

    // Technically we're not opening the directory for writing, which could be thought of as incorrect, but:
    // - We have a file within this directory, so it cannot be deleted.
    // - If anyone reads outdated entry metadata it shouldn't matter because open_or_incref will not open the file from
    //   scratch since we already have it open and that meta wouldn't be used.
    // - In the future we might reconsider open_or_incref to just take in a cluster number and read in the meta only after
    //   the file is open to prevent subtle race conditions.
    DirectoryEntry entry {};
    fs.locked_read(
        pure_cluster_value(m_identifier.file_directory_entry_cluster),
        m_identifier.file_directory_entry_offset_within_cluster,
        DirectoryEntry::size_in_bytes, &entry);

    // For now just size and first_cluster, todo modification time etc.
    entry.cluster_low = m_first_cluster & 0xFFFF;
    entry.cluster_high = m_first_cluster >> 16;
    entry.size = m_size;

    fs.locked_write(
        pure_cluster_value(m_identifier.file_directory_entry_cluster),
        m_identifier.file_directory_entry_offset_within_cluster,
        DirectoryEntry::size_in_bytes, &entry);
}

u32 FAT32::nth_cluster_in_chain(u32 start, u32 n)
{
    LOCK_GUARD(m_fat_cache_lock);

    u32 current = start;

    while (n--) {
        current = fat_entry_at(current);

        ASSERT(entry_type_of_fat_value(current) != FATEntryType::END_OF_CHAIN);
    }

    return current;
}

u32 FAT32::last_cluster_in_chain(u32 first)
{
    LOCK_GUARD(m_fat_cache_lock);

    u32 current = first;

    for (;;) {
        auto next = fat_entry_at(current);

        if (entry_type_of_fat_value(next) != FATEntryType::LINK)
            return current;

        current = next;
    }
}

FAT32::File* FAT32::open_or_incref(
    StringView name, File::Attributes attributes,
    const File::Identifier& identifier, u32 first_cluster, u32 size)
{
    FAT32_DEBUG << "opening file \"" << name << "\" cluster " << first_cluster;

    LOCK_GUARD(m_map_lock);
    auto it = m_identifier_to_file.find(identifier);

    if (it != m_identifier_to_file.end()) {
        auto& open_file = it->second;

        open_file->refcount++;

        FAT32_DEBUG << "file \"" << name << "\" was already open, new refcount " << open_file->refcount.load();

        return open_file->ptr;
    }

    FAT32_DEBUG << "opened file \"" << name << "\"";
    OpenFile* open_file = new OpenFile;
    open_file->ptr = new File(name, *this, attributes, identifier, first_cluster, size);
    open_file->refcount = 1;

    m_identifier_to_file[identifier] = open_file;
    return open_file->ptr;
}

Pair<ErrorCode, FAT32::File*> FAT32::open_file_from_path(StringView path, OnlyIf constraint)
{
    auto* cur_file = m_root_directory;
    File* next_file = nullptr;

    for (auto node : IterablePath(path)) {
        if (next_file) {
            close(*cur_file);
            cur_file = next_file;
            next_file = nullptr;
        }

        LOCK_GUARD(cur_file->lock());

        bool node_found = false;

        if ((cur_file->attributes() & File::Attributes::IS_DIRECTORY) != File::Attributes::IS_DIRECTORY) {
            close(*cur_file);
            return { ErrorCode::IS_FILE, nullptr };
        }

        Directory dir(*this, *cur_file, false);

        for (;;) {
            auto entry = dir.next_native();

            if (entry.empty())
                break;

            if (entry.name_view() != node)
                continue;

            next_file = open_or_incref(entry.name_view(), entry.attributes, entry.identifier(), entry.first_data_cluster, entry.size);

            node_found = true;
            break;
        }

        if (!node_found)
            break;
    }

    close(*cur_file);

    if (!next_file)
        return { ErrorCode::NO_SUCH_FILE, nullptr };

    if (constraint == OnlyIf::FILE && next_file->is_directory()) {
        close(*next_file);
        return { ErrorCode::IS_DIRECTORY, nullptr };
    } else if (constraint == OnlyIf::DIRECTORY && !next_file->is_directory()) {
        close(*next_file);
        return { ErrorCode::IS_FILE, nullptr };
    }

    return { ErrorCode::NO_ERROR, next_file };
}

DynamicArray<u32> FAT32::allocate_cluster_chain(u32 count, u32 link_to)
{
    ASSERT(count <= m_free_clusters);

    DynamicArray<u32> chain;
    chain.reserve(count);

    LOCK_GUARD(m_fat_cache_lock);

    auto hint = m_last_free_cluster.load();
    auto last = m_cluster_count;
    auto prev = link_to;

    for (size_t i = 0; i < 2; ++i) {
        for (size_t cluster = hint; cluster < last; ++cluster) {
            if (entry_type_of_fat_value(fat_entry_at(cluster)) != FATEntryType::FREE)
                continue;

            chain.append(cluster);

            if (prev != free_cluster)
                set_fat_entry_at(prev, cluster);

            prev = cluster;

            if (chain.size() == count) {
                m_free_clusters -= count;
                m_last_free_cluster = cluster + 1;
                set_fat_entry_at(cluster, m_end_of_chain);

                return chain;
            }
        }

        FAT32_DEBUG << "Cluster chain allocation after hint failed, trying before hint";
        last = hint;
        hint = reserved_cluster_count;
    }

    FAILED_ASSERTION("Failed to allocate enough clusters");
}

void FAT32::free_cluster_chain_starting_at(u32 first, FreeMode free_mode)
{
    LOCK_GUARD(m_fat_cache_lock);

    if (free_mode == FreeMode::KEEP_FIRST) {
        auto actual_first = fat_entry_at(first);
        set_fat_entry_at(first, m_end_of_chain);
        first = actual_first;
    }

    auto current = first;
    size_t freed_count = 0;

    for (;;) {
        auto next = fat_entry_at(current);
        set_fat_entry_at(current, free_cluster);
        freed_count++;

        if (entry_type_of_fat_value(next) == FATEntryType::END_OF_CHAIN) {
            FAT32_DEBUG << "freed " << freed_count << " clusters starting from " << first;
            return;
        }

        current = next;
    }
}

Pair<ErrorCode, FAT32::BaseDirectory*> FAT32::open_directory(StringView path)
{
    auto error_to_file = open_file_from_path(path, OnlyIf::DIRECTORY);

    return { error_to_file.first, new Directory(*this, *error_to_file.second, true) };
}

Pair<ErrorCode, FAT32::BaseFile*> FAT32::open(StringView path)
{
    auto error_to_file = open_file_from_path(path, OnlyIf::FILE);

    return { error_to_file.first, static_cast<BaseFile*>(error_to_file.second) };
}

ErrorCode FAT32::close(BaseFile& file)
{
    if (&file == m_root_directory) {
        FAT32_DEBUG << "tried to close root directory, ignored";
        return ErrorCode::NO_ERROR;
    }

    auto& f = static_cast<File&>(file);
    FAT32_DEBUG << "closing file \"" << f.name() << "\"";

    LOCK_GUARD(m_map_lock);

    auto it = m_identifier_to_file.find(f.identifier());
    if (it == m_identifier_to_file.end()) {
        String error_str;
        error_str << "FAT32: close() called on an unknown file " << file.name() << " cluster " << f.first_cluster();
        runtime::panic(error_str.c_string());
    }

    auto& open_file = it->second;
    if (--open_file->refcount == 0) {
        FAT32_DEBUG << "no more references for file \"" << f.name() << "\", closed";
        // Ideally we should call flush on any cached file clusters,
        // but it might be too expensive to fetch all the file clusters
        // and completely unnecessary if file wasn't read/written for example.
        m_identifier_to_file.remove(it);
    } else {
        FAT32_DEBUG << "file \"" << f.name() << "\" still has " << open_file->refcount.load() << " reference(s)";
    }

    return ErrorCode::NO_ERROR;
}

ErrorCode FAT32::close_directory(BaseDirectory& dir)
{
    delete &dir;

    return ErrorCode::NO_ERROR;
}

ErrorCode FAT32::remove_file(StringView path, OnlyIf constraint)
{
    FAT32_DEBUG << "removing file at " << path;

    auto* cur_file = m_root_directory;
    File* next_file = nullptr;

    auto itr_path = IterablePath(path);
    ErrorCode code = ErrorCode::NO_SUCH_FILE;

    for (;;) {
        auto node = *itr_path;
        ++itr_path;

        if (next_file) {
            close(*cur_file);
            cur_file = next_file;
            next_file = nullptr;
        }

        LOCK_GUARD(cur_file->lock());

        bool keep_going = false;

        if ((cur_file->attributes() & File::Attributes::IS_DIRECTORY) != File::Attributes::IS_DIRECTORY) {
            close(*cur_file);
            return { ErrorCode::IS_FILE };
        }

        Directory dir(*this, *cur_file, false);

        u32 previous_cluster = dir.first_cluster();

        for (;;) {
            auto entry = dir.next_native();

            if (entry.empty())
                break;

            if (entry.name_view() != node) {
                previous_cluster = entry.file_entry_cluster_1;
                continue;
            }

            auto identifier = entry.identifier();

            if (itr_path != itr_path.end()) {
                next_file = open_or_incref(entry.name_view(), entry.attributes, identifier, entry.first_data_cluster, entry.size);
                keep_going = true;
                break;
            }

            FAT32_DEBUG << "found the file to be deleted at " << entry.file_entry_cluster_1
                        << " with " << entry.sequence_count << " sequences";

            keep_going = false;

            bool is_directory = (entry.attributes & File::Attributes::IS_DIRECTORY) == File::Attributes::IS_DIRECTORY;

            if (is_directory && (constraint == OnlyIf::FILE)) {
                code = ErrorCode::IS_DIRECTORY;
                break;
            } else if (!is_directory && (constraint == OnlyIf::DIRECTORY)) {
                code = ErrorCode::IS_FILE;
                break;
            }

            {
                LOCK_GUARD(m_map_lock);
                if (m_identifier_to_file.contains(identifier)) {
                    code = ErrorCode::FILE_IS_BUSY;
                    break;
                }
            }

            if (is_directory && entry.first_data_cluster) {
                u8 first_byte = 0;

                locked_read(pure_cluster_value(entry.first_data_cluster), 0, 1, &first_byte);

                // Non-empty directory
                if (first_byte != DirectoryEntry::end_of_directory_mark) {
                    FAT32_DEBUG << "file " << path << " is a non-empty directory, cannot remove";
                    code = ErrorCode::FILE_IS_BUSY;
                    break;
                }
            }

            if (entry.first_data_cluster)
                free_cluster_chain_starting_at(entry.first_data_cluster, FreeMode::INCLUDING_FIRST);

            auto is_last_file_in_directory = [this](const Directory::NativeEntry& entry) {
                u32 current_cluster = entry.metadata_entry_cluster;
                u32 current_offset = entry.metadata_entry_offset_within_cluster;

                // Entry is the last in cluster, check if directory has other clusters
                if (current_offset == (bytes_per_cluster() - DirectoryEntry::size_in_bytes)) {
                    auto next_cluster = locked_fat_entry_at(current_cluster);

                    if (entry_type_of_fat_value(next_cluster) == FATEntryType::END_OF_CHAIN) {
                        return true;
                    } else if (entry_type_of_fat_value(next_cluster) == FATEntryType::LINK) {
                        u8 mark = 0;
                        locked_read(pure_cluster_value(next_cluster), 0, 1, &mark);
                        return mark == DirectoryEntry::end_of_directory_mark;
                    } else {
                        ASSERT_NEVER_REACHED();
                    }
                }

                // Directory has more entries ahead, look for end_of_directory mark
                u8 mark = 0;
                locked_read(pure_cluster_value(current_cluster), current_offset + DirectoryEntry::size_in_bytes, 1, &mark);
                return mark == DirectoryEntry::end_of_directory_mark;
            };

            bool last_file_in_directory = is_last_file_in_directory(entry);

            if (last_file_in_directory && entry.entry_offset_within_cluster == 0) {
                FAT32_DEBUG << "file " << path << " starts at cluster offset 0, we can delete the entire chain";

                // Edge case:
                // Directory entry is located entirely in its own cluster, so we can afford
                // to free the entire entry chain.
                free_cluster_chain_starting_at(previous_cluster, FreeMode::KEEP_FIRST);
                code = ErrorCode::NO_ERROR;
                break;
            }

            auto current_cluster = entry.file_entry_cluster_1;
            auto current_offset = entry.entry_offset_within_cluster;
            auto entries_to_delete = entry.sequence_count + 1; // sequences + 1 metadata
            FAT32_DEBUG << "file " << path << " has " << entries_to_delete
                        << " used directory entries, is_last: " << last_file_in_directory;

            DirectoryEntry deleted_entry {};

            if (last_file_in_directory)
                deleted_entry.mark_as_end_of_directory();
            else
                deleted_entry.mark_as_deleted();

            while (entries_to_delete--) {
                if (current_offset == m_bytes_per_cluster) {
                    // Entry spans multiple clusters and is also last, therefore
                    // we can afford not to mark last cluster entries as deleted
                    // since it will be freed anyways
                    if (last_file_in_directory)
                        break;

                    if (current_cluster == entry.file_entry_cluster_1) {
                        ASSERT(entry.file_entry_cluster_2);
                        current_cluster = entry.file_entry_cluster_2;
                    } else if (current_cluster == entry.file_entry_cluster_2) {
                        ASSERT(entry.file_entry_cluster_3);
                        current_cluster = entry.file_entry_cluster_3;
                    } else {
                        ASSERT_NEVER_REACHED();
                    }

                    current_offset = 0;
                }

                locked_write(pure_cluster_value(current_cluster), current_offset, DirectoryEntry::size_in_bytes, &deleted_entry);
                current_offset += DirectoryEntry::size_in_bytes;
            }

            if (last_file_in_directory)
                free_cluster_chain_starting_at(entry.file_entry_cluster_1, FreeMode::KEEP_FIRST);

            code = ErrorCode::NO_ERROR;
        }

        if (!keep_going)
            break;
    }

    close(*cur_file);

    return code;
}

ErrorCode FAT32::remove(StringView path)
{
    return remove_file(path, OnlyIf::FILE);
}

ErrorCode FAT32::remove_directory(StringView path)
{
    return remove_file(path, OnlyIf::DIRECTORY);
}

ErrorCode FAT32::create_file(StringView path, File::Attributes attributes)
{
    FAT32_DEBUG << "creating file " << path;

#define RETURN_WITH_CODE(code) \
    cur_file->lock().unlock(); \
    close(*cur_file);          \
    return code;

    auto* cur_file = m_root_directory;
    File* next_file = nullptr;

    auto itr_path = IterablePath(path);
    StringView current_node;
    String new_file_name;

    for (;;) {
        current_node = *itr_path;
        ++itr_path;

        if (next_file) {
            close(*cur_file);
            cur_file = next_file;
            next_file = nullptr;
        }

        cur_file->lock().lock();
        bool keep_going = false;

        if ((cur_file->attributes() & File::Attributes::IS_DIRECTORY) != File::Attributes::IS_DIRECTORY) {
            close(*cur_file);
            return ErrorCode::IS_FILE;
        }

        bool last_directory = itr_path != itr_path.end();
        Directory dir(*this, *cur_file, false);

        if (last_directory) {
            new_file_name = current_node;
            new_file_name.strip();
            new_file_name.rstrip('.');

            FAT32_DEBUG << "Original new file name \"" << current_node
                        << "\", after strip: \"" << new_file_name.to_view() << "\"";

            if (new_file_name.empty()) {
                RETURN_WITH_CODE(ErrorCode::BAD_FILENAME);
            }

            if (new_file_name.size() > File::max_name_length) {
                RETURN_WITH_CODE(ErrorCode::NAME_TOO_LONG);
            }
        }

        for (;;) {
            auto entry = dir.next_native();

            if (entry.empty())
                break;

            if (last_directory) {
                if (!case_insensitive_equals(new_file_name.to_view(), current_node))
                    continue;

                RETURN_WITH_CODE(ErrorCode::FILE_ALREADY_EXISTS);
            }

            if (!case_insensitive_equals(entry.name_view(), current_node))
                continue;

            next_file = open_or_incref(
                entry.name_view(), entry.attributes, entry.identifier(),
                entry.first_data_cluster, entry.size);
            keep_going = true;

            break;
        }

        if (!keep_going)
            break;

        cur_file->lock().unlock();
    }

    // Now we have the last directory open, we can proceed to allocating the directory slot
    Directory dir(*this, *cur_file, false);

    auto [name_length, extension_length] = length_of_name_and_extension(new_file_name.to_view());

    bool is_vfat = name_length > short_name_length || extension_length > short_extension_length;

    bool is_name_entirely_upper = false;
    bool is_name_entirely_lower = false;

    bool is_extension_entirely_upper = false;
    bool is_extension_entirely_lower = false;

    static constexpr char minimum_allowed_ascii_value = 0x20;

    static const char banned_characters[] = { '"', '*', '/', ':', '<', '>', '?', '\\', '|' };
    static constexpr size_t banned_characters_length = sizeof(banned_characters);

    static const char banned_but_allowed_in_vfat_characters[] = { '.', '+', ',', ';', '=', '[', ']' };
    static constexpr size_t banned_but_allowed_in_vfat_characters_length = sizeof(banned_but_allowed_in_vfat_characters);

    auto is_one_of = [](char c, const char* set, size_t length) -> bool {
        for (size_t i = 0; i < length; ++i) {
            if (c == set[i])
                return true;
        }

        return false;
    };

    bool contains_banned = false;

    for (char c : new_file_name) {
        contains_banned |= is_one_of(c, banned_characters, banned_characters_length);
        contains_banned |= c < minimum_allowed_ascii_value;

        if (contains_banned)
            break;
    }

    if (contains_banned) {
        RETURN_WITH_CODE(ErrorCode::BAD_FILENAME);
    }

    auto name_view = StringView(new_file_name.data(), name_length);

    if (!is_vfat) {
        auto count_lower_and_upper_chars = [](StringView node, size_t& lower_count, size_t& upper_count) {
            for (char c : node) {
                lower_count += String::is_lower(c);
                upper_count += String::is_upper(c);
            }
        };

        size_t name_lower_chars = 0;
        size_t name_upper_chars = 0;
        count_lower_and_upper_chars(name_view, name_lower_chars, name_upper_chars);

        is_name_entirely_lower = name_lower_chars && !name_upper_chars;
        is_name_entirely_upper = !name_lower_chars; // name_upper_chars don't matter

        if (extension_length) {
            auto extension_view = StringView(name_view.end() + 1, extension_length);
            size_t extension_lower_chars = 0;
            size_t extension_upper_chars = 0;
            count_lower_and_upper_chars(extension_view, extension_lower_chars, extension_upper_chars);

            is_extension_entirely_lower = extension_lower_chars && !extension_upper_chars;
            is_extension_entirely_upper = !extension_lower_chars; // extension_upper_chars don't matter
        }

        is_vfat = !((is_name_entirely_lower || is_name_entirely_upper) && (is_extension_entirely_lower || is_extension_entirely_upper));

        if (!is_vfat) {
            bool contains_banned_for_non_vfat = false;

            for (char c : name_view) {
                contains_banned_for_non_vfat |= is_one_of(c, banned_but_allowed_in_vfat_characters, banned_but_allowed_in_vfat_characters_length);

                if (contains_banned_for_non_vfat)
                    break;
            }

            if (extension_length) {
                auto extension_view = StringView(name_view.end() + 1, extension_length);

                for (char c : extension_view) {
                    contains_banned_for_non_vfat |= is_one_of(c, banned_but_allowed_in_vfat_characters, banned_but_allowed_in_vfat_characters_length);

                    if (contains_banned_for_non_vfat)
                        break;
                }
            }

            is_vfat = contains_banned_for_non_vfat;
        }
    }

    bool is_directory = (attributes & File::Attributes::IS_DIRECTORY) == File::Attributes::IS_DIRECTORY;

    auto write_normal_entry = [&](StringView name, StringView extension, Directory::Slot& slot, u32 first_cluster = free_cluster) {
        DirectoryEntry entry {};

        entry.cluster_low = first_cluster & 0xFFFF;
        entry.cluster_high = first_cluster >> 16;

        if (is_directory)
            entry.attributes = DirectoryEntry::Attributes::SUBDIRECTORY;

        if (is_name_entirely_lower)
            entry.case_info |= DirectoryEntry::CaseInfo::LOWERCASE_NAME;
        if (is_extension_entirely_lower)
            entry.case_info |= DirectoryEntry::CaseInfo::LOWERCASE_EXTENSION;

        size_t chars_copied = 0;
        for (char c : name)
            entry.filename[chars_copied++] = String::to_upper(c);

        auto name_padding = short_name_length - chars_copied;
        while (name_padding--)
            entry.filename[chars_copied++] = ' ';

        if (!extension.empty()) {
            chars_copied = 0;
            for (char c : extension)
                entry.extension[chars_copied++] = String::to_upper(c);
        }

        auto extension_padding = short_extension_length - chars_copied;
        while (extension_padding--)
            entry.extension[chars_copied++] = ' ';

        auto time = Time::now_readable();

        entry.created_ms = 0;

        uint16_t sec = time.second / 2;
        uint16_t min = time.minute;
        uint16_t hr = time.hour;
        uint16_t yr = time.year - 1980;
        uint16_t mt = time.month;
        uint16_t d = time.day;

        entry.created_time = (hr << 11) | (min << 5) | sec;
        entry.created_date = (yr << 9) | (mt << 5) | d;
        entry.last_accessed_date = entry.created_date;
        entry.last_modified_date = entry.created_date;
        entry.last_modified_time = entry.created_time;

        dir.write_one(&entry, slot);
    };

    auto generate_dot_and_dot_dot = [&](u32 first_cluster_of_directory, u32 first_cluster_of_previous_directory) {
        u32 clusters[3] {};
        clusters[0] = first_cluster_of_directory;

        Directory::Slot slot(0, clusters);

        write_normal_entry(".", "", slot, first_cluster_of_directory);
        write_normal_entry("..", "", slot, first_cluster_of_previous_directory);
    };

    if (!is_vfat) {
        auto slot = dir.allocate_entries(1);
        auto extension_view = StringView(name_view.end() + 1, extension_length);
        u32 first_cluster = 0;

        if (is_directory) {
            first_cluster = allocate_cluster_chain(1).last();
            generate_dot_and_dot_dot(first_cluster, cur_file->first_cluster());
        }

        write_normal_entry(name_view, extension_view, slot, first_cluster);

        RETURN_WITH_CODE(ErrorCode::NO_ERROR);
    }

    // Since name is VFAT it's always upper case
    is_name_entirely_lower = false;
    is_extension_entirely_lower = false;

    auto short_name = generate_short_name(new_file_name.to_view());

    for (;;) {
        auto entry = dir.next_native();

        if (entry.empty())
            break;

        if (StringView::from_char_array(entry.small_name) == short_name) {
            bool ok = false;
            short_name = next_short_name(short_name, ok);

            if (!ok) {
                // TODO: this actually means that there are too many files with similar names,
                //       but this is too rare of an error to invent a separate error code for it.
                //       Maybe I should?
                RETURN_WITH_CODE(ErrorCode::BAD_FILENAME);
            }

            dir.rewind();
        }
    }

    auto entries_to_write = ceiling_divide(new_file_name.size(), LongNameDirectoryEntry::characters_per_entry);

    // 1 extra for metadata
    auto slot = dir.allocate_entries(entries_to_write + 1);

    LongNameDirectoryEntry long_entry {};

    auto checksum = generate_short_name_checksum(short_name);
    long_entry.checksum = checksum;

    static constexpr size_t vfat_name_attributes = 0x0F;
    long_entry.attributes = vfat_name_attributes;

    struct NamePiece {
        const char* ptr;
        size_t characters;
    } pieces[max_sequence_number] {};

    auto generate_name_pieces = [](StringView name, NamePiece(&pieces)[20]) {
        size_t characters_left = name.size();
        const char* name_ptr = name.data();

        for (size_t i = 0;; i++) {
            auto characters_for_this_piece = min(LongNameDirectoryEntry::characters_per_entry, characters_left);
            characters_left -= characters_for_this_piece;

            pieces[i].ptr = name_ptr;
            pieces[i].characters = characters_for_this_piece;

            name_ptr += characters_for_this_piece;

            if (characters_left == 0)
                break;
        }
    };

    generate_name_pieces(new_file_name.to_view(), pieces);

    auto sequence_number = entries_to_write;

    auto write_long_directory_character = [](LongNameDirectoryEntry& entry, u16 character, size_t index) {
        if (index < LongNameDirectoryEntry::name_1_characters) {
            entry.name_1[index] = character;
        } else if (index < (LongNameDirectoryEntry::name_1_characters + LongNameDirectoryEntry::name_2_characters)) {
            index -= LongNameDirectoryEntry::name_1_characters;
            entry.name_2[index] = character;
        } else if (index < LongNameDirectoryEntry::characters_per_entry) {
            index -= LongNameDirectoryEntry::name_1_characters + LongNameDirectoryEntry::name_2_characters;
            entry.name_3[index] = character;
        } else {
            ASSERT_NEVER_REACHED();
        }
    };

    while (sequence_number--) {
        if ((sequence_number + 1) == entries_to_write)
            long_entry.make_last_logical_with_sequence_number(sequence_number + 1);
        else
            long_entry.sequence_number = sequence_number + 1;

        size_t characters_written = 0;
        auto& this_piece = pieces[sequence_number];

        for (; characters_written < this_piece.characters; ++characters_written)
            write_long_directory_character(long_entry, this_piece.ptr[characters_written], characters_written);

        if (characters_written < LongNameDirectoryEntry::characters_per_entry)
            write_long_directory_character(long_entry, 0, ++characters_written);

        while (characters_written < LongNameDirectoryEntry::characters_per_entry)
            write_long_directory_character(long_entry, 0xFFFF, ++characters_written);

        dir.write_one(&long_entry, slot);
    }

    auto short_name_view = StringView(short_name.data(), short_name_length);
    auto short_extension_view = StringView(short_name.data() + short_name_length + 1, short_extension_length);

    u32 first_cluster = 0;

    if (is_directory) {
        first_cluster = allocate_cluster_chain(1).last();
        generate_dot_and_dot_dot(first_cluster, cur_file->first_cluster());
    }

    write_normal_entry(short_name_view, short_extension_view, slot, first_cluster);

    RETURN_WITH_CODE(ErrorCode::NO_ERROR);
}
#undef RETURN_WITH_CODE

ErrorCode FAT32::create(StringView path, File::Attributes attributes)
{
    return create_file(path, attributes);
}

ErrorCode FAT32::create_directory(StringView path, File::Attributes attributes)
{
    return create_file(path, attributes);
}

ErrorCode FAT32::move(StringView, StringView)
{
    return ErrorCode::UNSUPPORTED;
}

void FAT32::sync()
{
    FAT32_DEBUG << "flushing all cached data...";

    static Mutex* sync_mutex = nullptr;

    if (!sync_mutex)
        sync_mutex = new Mutex();

    LOCK_GUARD(*sync_mutex);

    {
        LOCK_GUARD(m_fat_cache_lock);
        m_fat_cache->flush_all();
    }

    {
        LOCK_GUARD(m_data_lock);
        m_data_cache->flush_all();
    }

    auto fsinfo_sector = m_ebpb.fs_information_sector;
    if (!fsinfo_sector || fsinfo_sector == 0xFFFF)
        return;

    fsinfo_sector += lba_range().begin();

    static u32 last_known_free_cluster_count = 0;
    static u32 last_known_allocated_cluster = 0;

    if (last_known_free_cluster_count == m_free_clusters && last_known_allocated_cluster == (m_last_free_cluster - 1)) {
        FAT32_DEBUG << "FSINFO doesn't need an update, skipping sync for now";
        return;
    }

    last_known_free_cluster_count = m_free_clusters;
    last_known_allocated_cluster = m_last_free_cluster - 1;

    auto fsinfo_dma = MemoryManager::the().allocate_dma_buffer("FAT32 Sync"_sv, Page::size);

    auto info = associated_device().query_info();
    auto sectors_to_read = Page::size / info.logical_block_size;

    auto aligned_fsinfo_sector = fsinfo_sector & ~(sectors_to_read - 1);
    FAT32_DEBUG << "reading FSINFO, sector " << fsinfo_sector << " aligned at " << aligned_fsinfo_sector;

    auto read_req = StorageDevice::AsyncRequest::make_read(fsinfo_dma->virtual_range().begin(), { aligned_fsinfo_sector, sectors_to_read });
    associated_device().submit_request(read_req);
    read_req.wait();

    auto fsinfo_byte_offset = (fsinfo_sector - aligned_fsinfo_sector) * info.logical_block_size;
    FAT32_DEBUG << "FSINFO byte offset is " << fsinfo_byte_offset;

    auto& fsinfo = *Address(fsinfo_dma->virtual_range().begin() + fsinfo_byte_offset).as_pointer<FSINFO>();

    fsinfo.last_allocated_cluster = last_known_allocated_cluster;
    fsinfo.free_cluster_count = last_known_free_cluster_count;

    auto write_req = StorageDevice::AsyncRequest::make_write(fsinfo_dma->virtual_range().begin(), { aligned_fsinfo_sector, sectors_to_read });
    associated_device().submit_request(write_req);
    write_req.wait();
}

}