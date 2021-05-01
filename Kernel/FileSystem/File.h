#pragma once

#include "Multitasking/Mutex.h"

namespace kernel {

class FileSystem;

class File {
    MAKE_NONCOPYABLE(File);
    MAKE_NONMOVABLE(File);

public:
    static constexpr size_t max_name_length = 255;
    static constexpr size_t small_name_length = 12; // 8 + (.) + 3

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

    friend Attributes& operator|=(Attributes& l, Attributes r)
    {
        l = static_cast<Attributes>(static_cast<u32>(l) | static_cast<u32>(r));
        return l;
    }

    File(StringView name, FileSystem& filesystem, Attributes attributes)
        : m_filesystem(filesystem)
        , m_attributes(attributes)
    {
        copy_memory(name.data(), m_name, name.size());
        m_name[name.size()] = '\0';
    }

    virtual size_t read(void* buffer, size_t offset, size_t size) = 0;
    virtual size_t write(void* buffer, size_t offset, size_t size) = 0;

    bool is_directory() const { return (m_attributes & Attributes::IS_DIRECTORY) == Attributes::IS_DIRECTORY; }
    Attributes attributes() const { return m_attributes; }
    StringView name() const { return m_name; }
    FileSystem& fs() { return m_filesystem; }
    virtual size_t size() const = 0;

    Mutex& lock() { return m_lock; }

    virtual ~File() = default;

private:
    char m_name[max_name_length + 1];
    FileSystem& m_filesystem;
    Attributes m_attributes;
    Mutex m_lock;
};

}