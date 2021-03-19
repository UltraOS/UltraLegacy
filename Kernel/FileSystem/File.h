#pragma once

#include "Common/Lock.h"

namespace kernel {

class FileSystem;

class File {
public:
    enum class Attributes : u32 {
        IS_DIRECTORY = SET_BIT(0),
        // security-related stuff?...
    };

    friend Attributes operator&(Attributes l, Attributes r)
    {
        return static_cast<Attributes>(static_cast<u32>(l) & static_cast<u32>(r));
    }

    friend Attributes operator|(Attributes l, Attributes r)
    {
        return static_cast<Attributes>(static_cast<u32>(l) | static_cast<u32>(r));
    }

    File(StringView name, FileSystem& filesystem, Attributes attributes)
        : m_name(name)
        , m_filesystem(filesystem)
        , m_attributes(attributes)
    {
    }

    virtual size_t read(void* buffer, size_t offset, size_t size) = 0;
    virtual size_t write(void* buffer, size_t offset, size_t size) = 0;

    // TODO: this is a bit naive and doesn't use locking whatsoever
    bool is_directory() const { return (m_attributes & Attributes::IS_DIRECTORY) == Attributes::IS_DIRECTORY; }
    Attributes attributes() const { return m_attributes; }
    StringView name() const { return m_name; }
    FileSystem& fs() { return m_filesystem; }

    virtual ~File() = default;

private:
    StringView m_name;
    FileSystem& m_filesystem;
    Attributes m_attributes;
    InterruptSafeSpinLock m_lock;
};

}