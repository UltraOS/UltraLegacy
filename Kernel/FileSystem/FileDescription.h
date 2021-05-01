#pragma once

#include "Common/Types.h"
#include "File.h"
#include "Multitasking/Mutex.h"

namespace kernel {

class FileDescription {
public:
    enum class Mode {
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

    Mutex m_lock;
    File& m_file;
    Mode m_mode;
    size_t m_offset { 0 };
    bool m_is_closed { false };
};

}