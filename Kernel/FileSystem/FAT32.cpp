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

FAT32::BaseFile* FAT32::open(StringView)
{
    return nullptr;
}

void FAT32::close(BaseFile&)
{
}

bool FAT32::remove(StringView)
{
    return false;
}

void FAT32::create(StringView, File::Attributes)
{

}


void FAT32::move(StringView, StringView)
{

}

void FAT32::copy(StringView, StringView)
{

}


}