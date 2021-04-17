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
        File(StringView name, FileSystem& filesystem, Attributes attributes, u32 first_cluster, u32 size);

        size_t read(void* buffer, size_t offset, size_t size) override;
        size_t write(void* buffer, size_t offset, size_t size) override;

        u32 first_cluster() const { return m_first_cluster; }
        size_t size() const override { return m_size; }

        FAT32& fs_as_fat32() { return static_cast<FAT32&>(fs()); }

    private:
        u32 m_first_cluster { 0 };
        u32 m_size { 0 };
    };

    class Directory : public BaseDirectory {
    public:
        static constexpr size_t entry_size = DirectoryEntry::size_in_bytes;

        Directory(FileSystem& filesystem, File& directory_file, bool owns_file);

        struct NativeEntry : Entry {
            u32 first_file_cluster;
            u32 first_entry_cluster;
            u32 sequence_count;
            u32 offset_of_first_sequence;
            u32 starting_cluster;
        };

        NativeEntry next_native();
        Entry next() override;

        FAT32& fs_as_fat32() { return static_cast<FAT32&>(fs()); }

        u32 first_cluster() const { return static_cast<const File&>(file()).first_cluster(); }
        bool is_exhausted() const { return m_exhausted; }

        ~Directory()
        {
            if (m_owns_file)
                fs().close(file());
        }

    private:
        bool fetch_next(void* into);

    private:
        u32 m_current_cluster { 0 };
        size_t m_offset_within_cluster { 0 };
        bool m_exhausted { false };
        bool m_owns_file { false };
    };

    Pair<ErrorCode, BaseDirectory*> open_directory(StringView path) override;
    Pair<ErrorCode, BaseFile*> open(StringView path) override;
    ErrorCode close(BaseFile&) override;
    ErrorCode close_directory(BaseDirectory&) override;
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

    void locked_read(u64 block_index, size_t offset, size_t bytes, void* buffer);
    void locked_write(u64 block_index, size_t offset, size_t bytes, void* buffer);

    enum class OnlyIf {
        FILE,
        DIRECTORY
    };

    File* open_or_incref(StringView name, File::Attributes, u32 first_cluster, u64 size);
    Pair<ErrorCode, File*> open_file_from_path(StringView path, OnlyIf);

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
    u32 m_end_of_chain { 0 };

    DiskCache* m_fat_cache { nullptr };
    DiskCache* m_data_cache { nullptr };

    File* m_root_directory;

    struct OpenFile {
        File* ptr { nullptr };
        size_t refcount { 0 };
    };

    InterruptSafeSpinLock m_map_lock;
    InterruptSafeSpinLock m_fat_cache_lock;
    InterruptSafeSpinLock m_data_lock;
    Map<u32, OpenFile> m_cluster_to_file;
};

}