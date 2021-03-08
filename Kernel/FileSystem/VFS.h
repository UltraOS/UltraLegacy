#pragma once

#include "Common/Macros.h"
#include "Common/Map.h"
#include "Common/RefPtr.h"
#include "Core/Runtime.h"
#include "Drivers/Storage.h"
#include "FileSystem.h"
#include "File.h"
#include "FileDescription.h"

namespace kernel {

class VFS {
    MAKE_SINGLETON(VFS);

public:
    static void initialize()
    {
        ASSERT(s_instance == nullptr);
        s_instance = new VFS;
    }

    static VFS& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    RefPtr<FileDescription> open(StringView path, FileDescription::Mode);
    void close(FileDescription&);
    bool remove(StringView path);
    void create(StringView file_path, File::Attributes);
    
    void move(StringView path, StringView new_path);
    void copy(StringView path, StringView new_path);

    static bool is_valid_path(StringView);
    
private:
    void load_all_partitions(StorageDevice&);
    void load_mbr_partitions(StorageDevice&, Address virtual_mbr);

    String generate_prefix(StorageDevice&);

    static Optional<Pair<StringView, StringView>> split_prefix_and_path(StringView path);

private:
    Map<String, RefPtr<FileSystem>, Less<>> m_prefix_to_fs;
    Atomic<size_t> m_prefix_indices[static_cast<size_t>(StorageDevice::Info::MediumType::LAST)];

    static VFS* s_instance;
};

}