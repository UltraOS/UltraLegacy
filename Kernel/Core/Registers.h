#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

#ifdef ULTRA_32
struct PACKED RegisterState {
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
    u32 interrupt_number;
    u32 error_code;
    u32 eip;
    u32 cs;
    u32 eflags;
    u32 userspace_esp;
    u32 userspace_ss;

    u32 instruction_pointer() const { return eip; }
    u32 base_pointer() const { return ebp; }
};
#elif defined(ULTRA_64)
struct PACKED RegisterState {
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rbp;
    u64 rdi;
    u64 rsi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;
    u64 interrupt_number;
    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;

    u64 instruction_pointer() const { return rip; }
    u64 base_pointer() const { return rbp; }
};
#endif
}
