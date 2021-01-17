#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class ACPI {
public:
    struct PACKED RSDP {
        char signature[8];
        u8 checksum;
        char oemid[6];
        u8 revision;
        u32 rsdt_pointer;
    };

    static ACPI& the() { return s_instance; }

    void initialize();

private:
    bool find_rsdp();

private:
    RSDP* m_rsdp;

    static ACPI s_instance;
};
}