#include "Common/Logger.h"

#include "Core/Syscall.h"

#include "IDT.h"
#include "SyscallDispatcher.h"

namespace kernel {

SyscallDispatcher* SyscallDispatcher::s_instance;

void SyscallDispatcher::initialize()
{
    ASSERT(s_instance == nullptr);
    s_instance = new SyscallDispatcher;
}

SyscallDispatcher::SyscallDispatcher()
    : MonoInterruptHandler(vector_number, true)
{
}

void SyscallDispatcher::handle_interrupt(RegisterState& registers)
{
    // Syscall conventions:
    // Interrupt number: 0x80
    // Parameter passing order:
    // R/EAX -> syscall number
    // R/EBX, R/ECX, R/EDX, R/ESI, R/EDI, R/EBP -> syscall arguments, left -> right

#ifdef ULTRA_32
    const auto syscall_number = registers.eax;
    const auto arg0 = registers.ebx;
#elif defined(ULTRA_64)
    const auto syscall_number = registers.rax;
    const auto arg0 = registers.rbx;
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
