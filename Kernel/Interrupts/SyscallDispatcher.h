#pragma once

#include "Common/Types.h"

#include "Core/Registers.h"

namespace kernel {

class SyscallDispatcher {
    MAKE_STATIC(SyscallDispatcher);

public:
    static constexpr u16 syscall_index = 0x80;

    static constexpr u32 exit = 0;
    static constexpr u32 debug_log = 1;

    static void install();
    static void dispatch(RegisterState*) USED;
};
}
