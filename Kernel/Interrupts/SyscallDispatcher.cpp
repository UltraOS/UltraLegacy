#include "Common/Logger.h"

#include "Core/Syscall.h"

#include "Multitasking/Thread.h"

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
    // R/EBX, R/ECX, R/EDX -> syscall arguments, left -> right

    Thread::ScopedInvulnerability si;
    Syscall::invoke(registers);
}
}
