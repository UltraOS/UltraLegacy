#pragma once

namespace kernel {

class Syscall {
public:
    static void debug_log(const char* string);
};
}
