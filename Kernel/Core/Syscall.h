#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class Syscall {
    MAKE_STATIC(Syscall);

public:
    static void exit(size_t exit_code);
    static void debug_log(const char* string);
};
}
