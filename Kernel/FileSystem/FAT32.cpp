#include "FAT32.h"

namespace kernel {

FAT32::FAT32(StorageDevice& associated_device, LBARange lba_range)
    : FileSystem(associated_device, lba_range)
{
}

}