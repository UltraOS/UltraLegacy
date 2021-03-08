#pragma once

#include "FileSystem.h"
#include "File.h"

namespace kernel {

class FAT32 : public FileSystem {
public:
    using BaseFile = ::kernel::File;

    class File : public BaseFile {
    public:
        File(StringView name, FileSystem& filesystem, Attributes attributes, u32 first_cluster);

    private:
        u32 m_first_cluster;
    };

    FAT32(StorageDevice&, LBARange);

    BaseFile* open(StringView path) override;
    void close(BaseFile&) override;
    bool remove(StringView path) override;
    void create(StringView file_path, File::Attributes) override;

    void move(StringView path, StringView new_path) override;
    void copy(StringView path, StringView new_path) override;
};

}