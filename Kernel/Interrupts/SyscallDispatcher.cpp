#include "Common/Logger.h"

#include "Core/Syscall.h"

#include "Multitasking/Scheduler.h"
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
    // R/EAX -> return code, one of negated ErrorCode values

    auto& current_thread = *Thread::current();

    // We got killed by someone while running.
    // Yield so scheduler can deque & kill us.
    if (current_thread.should_die() || !current_thread.owner().is_alive())
        Scheduler::the().yield();

    // Prevent us from dying while running a syscall.
    current_thread.set_invulnerable(true);
    Syscall::invoke(registers);
    current_thread.set_invulnerable(false);

    // Got killed while executing the syscall.
    if (current_thread.should_die() || !current_thread.owner().is_alive())
        Scheduler::the().yield();
}
}
