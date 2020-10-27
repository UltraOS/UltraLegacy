#include "Common/Logger.h"

#include "Core/Syscall.h"

#include "IDT.h"
#include "SyscallDispatcher.h"

namespace kernel {

extern "C" void syscall_handler();

void SyscallDispatcher::install()
{
    IDT::the().register_user_interrupt_handler(syscall_index, &syscall_handler);
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
    const auto arg0 = registers->ebx;
#elif defined(ULTRA_64)
    const auto syscall_number = registers->rax;
    const auto arg0 = registers->rbx;
#endif

    switch (syscall_number) {
    case exit:
        Syscall::exit(arg0);
        break;
    case debug_log:
        // TODO: some safety to make sure this pointer doesn't lead to a ring 0 page fault :D
        Syscall::debug_log(reinterpret_cast<char*>(arg0));
        break;
    default:
        log() << "SyscallDispatcher: unknown syscall " << syscall_number;
    }
}
}
