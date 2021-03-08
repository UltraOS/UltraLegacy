#pragma once

#include "File.h"
#include "Common/Types.h"

namespace kernel {

class FileDescription {
public:
    enum class Mode
    {
        READ,
        WRITE,
    };

    FileDescription(File& file, Mode mode);
    size_t read(void* buffer, size_t size);
    size_t write(void* buffer, size_t size);
    void set_offset(size_t offset);

    ~FileDescription();

private:
    friend class VFS;
    File& raw_file() { return m_file; }
    void mark_as_closed();

    InterruptSafeSpinLock m_lock;
    File& m_file;
    Mode m_mode;
    size_t m_offset { 0 };
    bool m_is_closed { false };
};
    
}