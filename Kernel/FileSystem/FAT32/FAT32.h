#pragma once

#include "FileSystem/DiskCache.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/FileSystem.h"
#include "Structures.h"

namespace kernel {

class FAT32 : public FileSystem {
public:
    using BaseFile = ::kernel::File;
    using BaseDirectory = ::kernel::Directory;

    static RefPtr<FileSystem> create(StorageDevice& associated_device, LBARange lba_range);

    class File : public BaseFile {
    public:
        File(StringView name, FileSystem& filesystem, Attributes attributes, u32 first_cluster);

    private:
        u32 m_first_cluster;
    };

    class Directory : public BaseDirectory {
    public:
        static constexpr size_t entry_size = DirectoryEntry::size_in_bytes;

        Directory(FileSystem& filesystem, u32 first_cluster);

        Entry next() override;

        FAT32& fs_as_fat32() { return static_cast<FAT32&>(fs()); }

    private:
        bool fetch_next(void* into);
        bool fetch_next_raw(void* into);

    private:
        u32 m_first_cluster { 0 };
        u32 m_current_cluster { 0 };
        size_t m_offset_within_cluster { 0 };
        bool m_exhausted { false };
    };

    Pair<ErrorCode, BaseDirectory*> open_directory(StringView path) override;
    Pair<ErrorCode, BaseFile*> open(StringView path) override;
    ErrorCode close(BaseFile&) override;
    ErrorCode remove(StringView path) override;
    ErrorCode create(StringView file_path, File::Attributes) override;

    ErrorCode move(StringView path, StringView new_path) override;
    ErrorCode copy(StringView path, StringView new_path) override;

private:
    bool try_initialize();
    FAT32(StorageDevice&, LBARange);
    bool parse_fsinfo(FSINFO& fsinfo);
    void calculate_capacity();
    u32 fat_entry_at(u32);
    void set_fat_entry_at(u32 index, u32 value);

    enum class FATEntryType {
        FREE,
        RESERVED,
        BAD,
        END_OF_CHAIN,
        LINK,
    };
    FATEntryType entry_type_of_fat_value(u32);
    static u32 pure_cluster_value(u32);

    u32 bytes_per_cluster() const { return m_bytes_per_cluster; }

    static constexpr u32 free_cluster = 0x00000000;
    static constexpr u32 bad_cluster = 0x0FFFFFF7;
    static constexpr u32 end_of_chain_min_cluster = 0x0FFFFFF8;
    static constexpr u32 reserved_cluster_count = 2;

private:
    EBPB m_ebpb;
    size_t m_last_free_cluster { 0 };
    size_t m_free_clusters { 0 };
    size_t m_cluster_count { 0 };
    u32 m_bytes_per_cluster { 0 };
    DiskCache* m_fat_cache { nullptr };
    DiskCache* m_data_cache { nullptr };
    u32 m_end_of_chain { 0 };
};

}