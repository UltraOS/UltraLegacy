#pragma once

#include "VFS.h"

namespace kernel {

class FileSystem {
    MAKE_NONCOPYABLE(FileSystem);

public:
    FileSystem(StorageDevice& associated_device, LBARange lba_range)
        : m_associated_device(associated_device)
        , m_lba_range(lba_range)
    {
    }

    StorageDevice& associated_device() { return m_associated_device; }
    LBARange lba_range() const { return m_lba_range; }

private:
    StorageDevice& m_associated_device;
    LBARange m_lba_range;
};

}