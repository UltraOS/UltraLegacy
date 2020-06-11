#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel {
    struct PACKED RegisterState
    {
        u32 ss;
        u32 gs;
        u32 fs;
        u32 es;
        u32 ds;
        u32 edi;
        u32 esi;
        u32 ebp;
        u32 esp;
        u32 ebx;
        u32 edx;
        u32 ecx;
        u32 eax;
        u32 error_code;
        u32 eip;
        u32 cs;
        u32 eflags;
        u32 userspace_esp;
        u32 userspace_ss;
    };
}
