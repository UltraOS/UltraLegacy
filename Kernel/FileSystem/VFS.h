#pragma once

#include "Common/Macros.h"
#include "Common/Map.h"
#include "Common/RefPtr.h"
#include "Core/ErrorCode.h"
#include "Core/Runtime.h"
#include "Directory.h"
#include "Drivers/Storage.h"
#include "File.h"
#include "FileDescription.h"
#include "FileSystem.h"

namespace kernel {

class VFS {
    MAKE_SINGLETON(VFS);

public:
    static constexpr StringView boot_fs_prefix = ""_sv;

    static void initialize()
    {
        ASSERT(s_instance == nullptr);
        Thread::ScopedInvulnerability i;
        s_instance = new VFS;

        Process::create_supervisor(sync_thread, "sync task");
    }

    [[noreturn]] static void sync_thread();

    static VFS& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    ErrorOr<RefPtr<FileDescription>> open(StringView path, FileMode);
    ErrorCode close(FileDescription&);
    ErrorCode remove(StringView path);
    ErrorCode remove_directory(StringView path);
    ErrorCode create(StringView file_path);
    ErrorCode create_directory(StringView file_path);

    ErrorCode move(StringView path, StringView new_path);

    void sync_all();

private:
    void load_all_partitions(StorageDevice&);
    void load_mbr_partitions(StorageDevice&, Address virtual_mbr, size_t sector_offset = 0);

    String generate_prefix(StorageDevice&);

private:
    Map<String, RefPtr<FileSystem>, Less<>> m_prefix_to_fs;
    Atomic<size_t> m_prefix_indices[static_cast<size_t>(StorageDevice::Info::MediumType::LAST)];

    static VFS* s_instance;
};

}