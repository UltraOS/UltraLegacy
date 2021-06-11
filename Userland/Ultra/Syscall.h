#pragma once

#include <Shared/Syscalls.h>

enum {
#define SYSCALL(name) SYSCALL_## name,
    ENUMERATE_SYSCALLS
#undef SYSCALL
};

// raw syscall interface, no errno or anything like that
long syscall_0(long function);
long syscall_1(long function, long arg0);
long syscall_2(long function, long arg0, long arg1);
long syscall_3(long function, long arg0, long arg1, long arg2);
