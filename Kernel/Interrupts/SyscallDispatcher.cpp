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
    // R/EAX -> syscall number
    // R/EBX, R/ECX, R/EDX, R/ESI, R/EDI, R/EBP -> syscall arguments, left -> right

#ifdef ULTRA_32
    const auto syscall_number = registers->eax;
    const auto arg0           = registers->ebx;
#elif defined(ULTRA_64)
    const auto syscall_number = registers->rax;
    const auto arg0           = registers->rbx;
#endif

    switch (syscall_number) {
    case exit: Syscall::exit(arg0); break;
    case debug_log:
        // TODO: some safety to make sure this pointer doesn't lead to a ring 0 page fault :D
        Syscall::debug_log(reinterpret_cast<char*>(arg0));
        break;
    default: log() << "SyscallDispatcher: unknown syscall " << syscall_number;
    }
}

#ifdef ULTRA_32
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
#elif defined(ULTRA_64)
asm(".globl syscall\n"
    "syscall:\n"
    "    pushq $0\n"
    "    pushq %rax\n"
    "    pushq %rbx\n"
    "    pushq %rcx\n"
    "    pushq %rdx\n"
    "    pushq %rsi\n"
    "    pushq %rdi\n"
    "    pushq %rbp\n"
    "    pushq %r8\n"
    "    pushq %r9\n"
    "    pushq %r10\n"
    "    pushq %r11\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "    movq %rsp, %rdi\n"
    "    cld\n"
    "    call _ZN6kernel17SyscallDispatcher8dispatchEPNS_13RegisterStateE\n"
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %r11\n"
    "    popq %r10\n"
    "    popq %r9\n"
    "    popq %r8\n"
    "    popq %rbp\n"
    "    popq %rdi\n"
    "    popq %rsi\n"
    "    popq %rdx\n"
    "    popq %rcx\n"
    "    popq %rbx\n"
    "    popq %rax\n"
    "    add $0x8, %rsp\n"
    "    iretq\n");
#endif
}
