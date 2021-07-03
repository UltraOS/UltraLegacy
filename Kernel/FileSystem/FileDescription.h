#pragma once

#include "Common/Types.h"
#include "File.h"
#include "Multitasking/Mutex.h"

namespace kernel {

class FileDescription {
public:
    FileDescription(File& file, FileMode mode);
    size_t read(void* buffer, size_t size);
    size_t write(const void* buffer, size_t size);
    ErrorOr<size_t> set_offset(size_t offset, SeekMode mode);

    File& underlying_file() { return m_file; }

    ~FileDescription();

private:
    friend class VFS;
    File& raw_file() { return m_file; }
    void mark_as_closed();

    Mutex m_lock;
    File& m_file;
    FileMode m_mode;
    size_t m_offset { 0 };
    bool m_is_closed { false };
};

}