#pragma once

#include "Common/Macros.h"
#include "Common/Map.h"
#include "Common/RefPtr.h"
#include "Core/Runtime.h"
#include "Drivers/Storage.h"

namespace kernel {

class FileSystem;
class StorageDevice;

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

private:
    void load_all_partitions(StorageDevice&);
    void load_mbr_partitions(StorageDevice&, Address virtual_mbr);

    String generate_prefix(StorageDevice&);

private:
    Map<String, RefPtr<FileSystem>> m_prefix_to_fs;
    Atomic<size_t> m_prefix_indices[static_cast<size_t>(StorageDevice::Info::MediumType::LAST)];

    static VFS* s_instance;
};

}