#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/ErrorCode.h"
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

#define SYSCALL(name) static ErrorOr<ptr_t> name(RegisterState&);
    ENUMERATE_SYSCALLS
#undef SYSCALL

    static void invoke(RegisterState&);
private:
    static ErrorOr<ptr_t>(*s_table[static_cast<size_t>(NumberOf::MAX) + 1])(RegisterState&);
};

#define SYSCALL_IMPLEMENTATION(name) ErrorOr<ptr_t> Syscall::name([[maybe_unused]] RegisterState& registers)

}
