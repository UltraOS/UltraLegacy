#pragma once

#include "FileSystem.h"

namespace kernel {

class FAT32 : public FileSystem {
public:
    FAT32(StorageDevice&, LBARange);
};

}