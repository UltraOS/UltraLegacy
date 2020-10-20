#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class Syscall {
    MAKE_STATIC(Syscall);

public:
    static void exit(u8 exit_code);
    static void debug_log(const char* string);
};
}
