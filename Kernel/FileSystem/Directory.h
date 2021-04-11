#pragma once

#include "File.h"

namespace kernel {

class FileSystem;

class Directory
{
public:
    Directory(FileSystem& fs)
        : m_fs(fs)
    {
    }

    struct Entry {
        char name[File::max_name_length + 1];
        char small_name[File::small_name_length + 1];
        File::Attributes attributes;
        u64 size;

        StringView name_view() const { return StringView(name); }
        StringView small_name_view() const { return StringView(small_name); }
        bool empty() const { return name[0] == '\0'; }
    };

    virtual Entry next() = 0;

    FileSystem& fs() { return m_fs; }

    ~Directory() = default;

private:
    FileSystem& m_fs;
};


}