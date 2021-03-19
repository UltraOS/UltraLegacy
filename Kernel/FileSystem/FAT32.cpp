#include "FAT32.h"

namespace kernel {

FAT32::FAT32(StorageDevice& associated_device, LBARange lba_range)
    : FileSystem(associated_device, lba_range)
{
}

FAT32::File::File(StringView name, FileSystem& filesystem, Attributes attributes, u32 first_cluster)
    : BaseFile(name, filesystem, attributes)
    , m_first_cluster(first_cluster)
{
}

Pair<ErrorCode, FAT32::BaseFile*> FAT32::open(StringView)
{
    return { ErrorCode::UNSUPPORTED, nullptr };
}

ErrorCode FAT32::close(BaseFile&)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::remove(StringView)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::create(StringView, File::Attributes)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::move(StringView, StringView)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::copy(StringView, StringView)
{
    return ErrorCode::UNSUPPORTED;
}

}