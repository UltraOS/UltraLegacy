#include "SyscallDispatcher.h"
#include "Core/Syscall.h"
#include "IDT.h"

namespace kernel {

extern "C" void syscall();

void SyscallDispatcher::initialize()
{
    IDT::the().register_user_interrupt_handler(syscall_index, &syscall);
}

void SyscallDispatcher::dispatch(RegisterState* registers)
{
    // Syscall conventions:
    // Interrupt number: 0x80
    // Parameter passing order:
    // EAX -> syscall number
    // EBX, ECX, EDX, ESI, EDI, EBP -> syscall arguments, left -> right

    switch (registers->eax) {
    case exit: Syscall::exit(registers->ebx); break;
    case debug_log:
        // TODO: some safety to make sure this pointer doesn't lead to a ring 0 page fault :D
        Syscall::debug_log(reinterpret_cast<char*>(registers->ebx));
        break;
    default: log() << "SyscallDispatcher: unknown syscall " << registers->eax;
    }
}

asm(".globl syscall\n"
    "syscall:\n"
    "    pushl $0\n"
    "    pusha\n"
    "    pushl %ds\n"
    "    pushl %es\n"
    "    pushl %fs\n"
    "    pushl %gs\n"
    "    pushl %ss\n"
    "    mov $0x10, %ax\n"
    "    mov %ax, %ds\n"
    "    mov %ax, %es\n"
    "    cld\n"
    "    pushl %esp\n"
    "    call _ZN6kernel17SyscallDispatcher8dispatchEPNS_13RegisterStateE\n"
    "    add $0x8, %esp \n"
    "    popl %gs\n"
    "    popl %fs\n"
    "    popl %es\n"
    "    popl %ds\n"
    "    popa\n"
    "    add $0x4, %esp\n"
    "    iret\n");
}
