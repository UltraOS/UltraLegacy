#pragma once

#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"
#include "Structures.h"

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

    Pair<ErrorCode, BaseFile*> open(StringView path) override;
    ErrorCode close(BaseFile&) override;
    ErrorCode remove(StringView path) override;
    ErrorCode create(StringView file_path, File::Attributes) override;

    ErrorCode move(StringView path, StringView new_path) override;
    ErrorCode copy(StringView path, StringView new_path) override;

private:
    EBPB m_ebpb;
};

}