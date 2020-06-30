#pragma once

#include "Common/Types.h"

namespace kernel {

class Syscall {
public:
    static void exit(u8 exit_code);
    static void debug_log(const char* string);
};
}
