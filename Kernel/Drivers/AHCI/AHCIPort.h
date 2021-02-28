#pragma once

#include "Drivers/Storage.h"

namespace kernel {

class AHCI;

class AHCIPort : public StorageDevice {
public:
    AHCIPort(AHCI& parent, size_t index);

    void read_synchronous(Address into_virtual_address, LBARange) override;
    void write_synchronous(Address from_virtual_address, LBARange) override;
    Info query_info() const override;

    StringView device_type() const override { return "AHCI Port"_sv; }
    StringView device_model() const override { return device_type(); }

private:
    AHCI& m_controller;
    size_t m_index;
};

}