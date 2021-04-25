#pragma once

#include "FileSystem/DiskCache.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/FileSystem.h"
#include "Multitasking/Mutex.h"
#include "Structures.h"

namespace kernel {

class FAT32 : public FileSystem {
public:
    using BaseFile = ::kernel::File;
    using BaseDirectory = ::kernel::Directory;

    static RefPtr<FileSystem> create(StorageDevice& associated_device, LBARange lba_range);

    class File : public BaseFile {
    public:
        // Used to uniquely identify a file within the volume.
        // An obvious and simpler choice would be to use file's first data cluster
        // as the unique identifier, but FAT allows files of size 0 to have no associated
        // clusters therefore we could have any number of files with the first data cluster of 0.
        struct Identifier {
            u32 file_directory_entry_cluster;
            u32 file_directory_entry_offset_within_cluster;

            friend bool operator<(const Identifier& lhs, const Identifier& rhs)
            {
                if (lhs.file_directory_entry_cluster != rhs.file_directory_entry_cluster)
                    return lhs.file_directory_entry_cluster < rhs.file_directory_entry_cluster;

                return lhs.file_directory_entry_offset_within_cluster < rhs.file_directory_entry_offset_within_cluster;
            }
        };

        File(StringView name, FileSystem& filesystem, Attributes attributes, const Identifier&, u32 first_data_cluster, u32 size);

        size_t read(void* buffer, size_t offset, size_t size) override;
        size_t write(void* buffer, size_t offset, size_t size) override;

        void flush_meta_modifications();

        u32 first_cluster() const { return m_first_cluster; }
        const Identifier& identifier() const { return m_identifier; }

        u32 compute_last_cluster()
        {
            if (m_first_cluster == free_cluster)
                return free_cluster;

            if (!m_last_cluster)
                m_last_cluster = fs_as_fat32().last_cluster_in_chain(m_first_cluster);

            return m_last_cluster;
        }

        void set_first_cluster(u32 first_cluster)
        {
            m_first_cluster = first_cluster;
            m_is_dirty = true;
        }

        void set_last_cluster(u32 last_cluster)
        {
            m_is_dirty = true;
            m_last_cluster = last_cluster;
        }

        void set_size(size_t size)
        {
            m_is_dirty = true;
            m_size = size;
        }

        bool is_dirty() const { return m_is_dirty; }
        void mark_clean() { m_is_dirty = false; }
        void mark_dirty() { m_is_dirty = true; }

        size_t size() const override { return m_size; }

        FAT32& fs_as_fat32() { return static_cast<FAT32&>(fs()); }

    private:
        Identifier m_identifier { };
        u32 m_first_cluster { 0 };
        u32 m_last_cluster { 0 };
        u32 m_size { 0 };
        bool m_is_dirty { false };
    };

    class Directory : public BaseDirectory {
    public:
        static constexpr size_t entry_size = DirectoryEntry::size_in_bytes;

        Directory(FileSystem& filesystem, File& directory_file, bool owns_file);

        struct NativeEntry : Entry {
            u32 sequence_count;
            u32 entry_offset_within_cluster;

            // Worst case:
            // 512 bytes per cluster, 21 entries for VFAT name (255 chars), 1st entry starts at byte 480
            //  1    2    3    <-- clusters
            // ---- ---- ----
            //    --------     <-- VFAT directory entry
            u32 file_entry_cluster_1;
            u32 file_entry_cluster_2;
            u32 file_entry_cluster_3;

            u32 first_data_cluster;
            u32 metadata_entry_cluster; // same as file_entry_cluster_1 if not VFAT
            u32 metadata_entry_offset_within_cluster; // same as entry_offset_within_cluster if not VFAT

            File::Identifier identifier() const { return { metadata_entry_cluster, metadata_entry_offset_within_cluster }; }
        };

        NativeEntry next_native();
        Entry next() override;

        void rewind() override
        {
            m_current_cluster = file_as_fat32().first_cluster();
            m_offset_within_cluster = 0;
            m_exhausted = false;
        }

        FAT32& fs_as_fat32() { return static_cast<FAT32&>(fs()); }
        File& file_as_fat32() { return static_cast<File&>(file()); }

        u32 first_cluster() const { return static_cast<const File&>(file()).first_cluster(); }
        bool is_exhausted() const { return m_exhausted; }

        ~Directory()
        {
            if (m_owns_file)
                fs().close(file());
        }

    private:
        bool fetch_next(void* into);

        struct Slot {
            u32 offset_within_cluster;
            u32 cluster_1;
            u32 cluster_2;
            u32 cluster_3;
        };
        Pair<ErrorCode, Slot> allocate_file_slot(StringView);

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
    ErrorCode remove_directory(StringView path) override;
    ErrorCode create(StringView file_path, File::Attributes) override;
    ErrorCode create_directory(StringView file_path, File::Attributes) override;

    ErrorCode move(StringView path, StringView new_path) override;
    ErrorCode copy(StringView path, StringView new_path) override;

    void sync() override;

private:
    bool try_initialize();
    FAT32(StorageDevice&, LBARange);
    bool parse_fsinfo(FSINFO& fsinfo);
    void calculate_capacity();

    u32 locked_fat_entry_at(u32);
    void locked_set_fat_entry_at(u32 index, u32 value);

    u32 fat_entry_at(u32);
    void set_fat_entry_at(u32 index, u32 value);

    void locked_read(u64 block_index, size_t offset, size_t bytes, void* buffer);
    void locked_write(u64 block_index, size_t offset, size_t bytes, void* buffer);
    void locked_zero_fill(u64 block_index, size_t count);

    u32 nth_cluster_in_chain(u32 start, u32 n);
    u32 last_cluster_in_chain(u32);
    DynamicArray<u32> allocate_cluster_chain(u32, u32 link_to = free_cluster);

    enum class FreeMode {
        KEEP_FIRST,
        INCLUDING_FIRST
    };
    void free_cluster_chain_starting_at(u32 first, FreeMode);

    enum class OnlyIf {
        FILE,
        DIRECTORY
    };
    Pair<ErrorCode, File*> open_file_from_path(StringView path, OnlyIf);
    ErrorCode remove_file(StringView, OnlyIf);
    ErrorCode create_file(StringView, File::Attributes);

    File* open_or_incref(StringView name, File::Attributes, const File::Identifier&, u32 first_cluster, u32 size);

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
    Atomic<size_t> m_last_free_cluster { 0 };
    Atomic<size_t> m_free_clusters { 0 };
    size_t m_cluster_count { 0 };
    u32 m_bytes_per_cluster { 0 };
    u32 m_end_of_chain { 0 };

    DiskCache* m_fat_cache { nullptr };
    DiskCache* m_data_cache { nullptr };

    File* m_root_directory;

    struct OpenFile {
        File* ptr { nullptr };
        Atomic<size_t> refcount { 0 };
    };

    Mutex m_map_lock;
    Mutex m_fat_cache_lock;
    Mutex m_data_lock;
    Map<File::Identifier, OpenFile*> m_identifier_to_file;
};

}