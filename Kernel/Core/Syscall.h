#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "Shared/Syscalls.h"

namespace kernel {

class Syscall {
    MAKE_STATIC(Syscall);
public:
    enum class NumberOf {
#define SYSCALL(name) name,
        ENUMERATE_SYSCALLS
#undef SYSCALL
    };

#define SYSCALL(name) static void name(RegisterState&);
    ENUMERATE_SYSCALLS
#undef SYSCALL

    static void invoke(RegisterState&);
private:
    static void(*s_table[static_cast<size_t>(NumberOf::MAX) + 1])(RegisterState&);
};

#define SYSCALL_IMPLEMENTATION(name) void Syscall::name([[maybe_unused]] RegisterState& registers)

}
