#include "SyscallDispatcher.h"
#include "IDT.h"
#include "Core/Syscall.h"

namespace kernel {

extern "C" void syscall();

void SyscallDispatcher::initialize()
{
    IDT::the().register_user_interrupt_handler(syscall_index, &syscall);
}

void SyscallDispatcher::dispatch(RegisterState registers)
{
    switch (registers.eax)
    {
    case exit:
        break;
    case debug_log:
        // TODO: some safety to make sure this pointer doesn't lead to a ring 0 pagefault :D
        Syscall::debug_log(reinterpret_cast<char*>(registers.esi));
        break;
    default:
        log() << "SyscallDispatcher: unknown syscall " << registers.eax;
    }
}

asm(".globl syscall\n"                                                                                             \
    "syscall:\n"                                                                                                   \
    "    pushl $0\n"                                                                                               \
    "    pusha\n"                                                                                                  \
    "    pushl %ds\n"                                                                                              \
    "    pushl %es\n"                                                                                              \
    "    pushl %fs\n"                                                                                              \
    "    pushl %gs\n"                                                                                              \
    "    pushl %ss\n"                                                                                              \
    "    mov $0x10, %ax\n"                                                                                         \
    "    mov %ax, %ds\n"                                                                                           \
    "    mov %ax, %es\n"                                                                                           \
    "    cld\n"                                                                                                    \
    "    call _ZN6kernel17SyscallDispatcher8dispatchENS_13RegisterStateE\n"                                        \
    "    add $0x4, %esp \n"                                                                                        \
    "    popl %gs\n"                                                                                               \
    "    popl %fs\n"                                                                                               \
    "    popl %es\n"                                                                                               \
    "    popl %ds\n"                                                                                               \
    "    popa\n"                                                                                                   \
    "    add $0x4, %esp\n"                                                                                         \
    "    iret\n");
}
