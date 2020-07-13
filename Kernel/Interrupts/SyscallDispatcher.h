#pragma once

#include "Interrupts/Common.h"

namespace kernel {

class SyscallDispatcher {
public:
    static constexpr u16 syscall_index = 0x80;

    static constexpr u32 exit      = 0;
    static constexpr u32 debug_log = 1;

    static void initialize();
    static void dispatch(RegisterState*) USED;
};
}
