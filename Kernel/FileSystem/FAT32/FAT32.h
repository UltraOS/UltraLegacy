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
        Directory(FileSystem& filesystem, u32 first_cluster);

        FAT32& fs_as_fat32() { return static_cast<FAT32&>(fs()); }

    private:
        size_t m_entry_offset { 0 };
        size_t m_entry_offset_within_cluster { 0 };
        u32 m_first_cluster;
        u32 m_previous_cluster;
        u32 m_current_cluster;
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
    u32 get_fat_entry(u32);
    void put_fat_entry(u32 index, u32 value);

    static constexpr u32 free_cluster = 0x00000000;

private:
    EBPB m_ebpb;
    size_t m_last_free_cluster { 0 };
    size_t m_free_clusters { 0 };
    size_t m_cluster_count { 0 };
    DiskCache* m_fat_cache { nullptr };
    DiskCache* m_data_cache { nullptr };
    u32 m_end_of_chain { 0 };
};

}