#pragma once

#include "Device.h"
#include "Memory/MemoryManager.h"

namespace kernel {

class StorageDevice : public Device {
public:
    StorageDevice()
        : Device(Category::STORAGE)
    {
    }

    struct Info {
        u64 lba_count;
        size_t lba_size;
        size_t optimal_read_size;
    };

    virtual void read_synchronous(Address into_virtual_address, size_t first_lba, size_t lba_count) = 0;
    virtual void write_synchronous(Address from_virtual_address, size_t first_lba, size_t lba_count) = 0;
    virtual Info query_info() const = 0;
};

}