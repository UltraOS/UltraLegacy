#pragma once

#include "File.h"

namespace kernel {

class FileSystem;

class Directory
{
    MAKE_NONCOPYABLE(Directory);
    MAKE_NONMOVABLE(Directory);
public:
    Directory(FileSystem& fs, File& associated_file)
        : m_fs(&fs)
        , m_file(&associated_file)
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
    virtual void rewind() = 0;

    FileSystem& fs() { return *m_fs; }
    const FileSystem& fs() const { return *m_fs; }

    File& file() { return *m_file; }
    const File& file() const { return *m_file; }

    virtual ~Directory() = default;

private:
    FileSystem* m_fs;
    File* m_file;
};


}