#include "Common/Logger.h"

#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"
#include "Multitasking/Sleep.h"

#include "FileSystem/VFS.h"

#include "Syscall.h"

namespace kernel {

void(*Syscall::s_table[static_cast<size_t>(Syscall::NumberOf::MAX) + 1])(RegisterState&) =
{
#define SYSCALL(name) &Syscall::name,
ENUMERATE_SYSCALLS
#undef SYSCALL
};

#ifdef ULTRA_32
#define RETVAL(regs) regs.eax
#define FUNCTION(regs) regs.eax
#define ARG0(regs) regs.ebx
#define ARG1(regs) regs.ecx
#define ARG2(regs) regs.edx
#elif defined(ULTRA_64)
#define RETVAL(regs) regs.rax
#define FUNCTION(regs) regs.rax
#define ARG0(regs) regs.rbx
#define ARG1(regs) regs.rcx
#define ARG2(regs) regs.rdx
#endif

void Syscall::invoke(RegisterState& registers)
{
    auto function = FUNCTION(registers);

    if (function >= static_cast<size_t>(NumberOf::MAX)) {
        RETVAL(registers) = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    s_table[function](registers);
}

SYSCALL_IMPLEMENTATION(EXIT)
{
    Thread::current()->set_invulnerable(false);
    Scheduler::the().exit(ARG0(registers));
}

SYSCALL_IMPLEMENTATION(OPEN)
{
     // TODO
}

SYSCALL_IMPLEMENTATION(CLOSE)
{
     // TODO
}

SYSCALL_IMPLEMENTATION(READ)
{
     // TODO
}

SYSCALL_IMPLEMENTATION(WRITE)
{
     // TODO
}

SYSCALL_IMPLEMENTATION(SEEK)
{
     // TODO
}

SYSCALL_IMPLEMENTATION(VIRTUAL_ALLOC)
{
    auto range = AddressSpace::current().allocator().allocate(ARG0(registers));

    RETVAL(registers) = range.begin();
}

SYSCALL_IMPLEMENTATION(VIRTUAL_FREE)
{
    AddressSpace::current().allocator().deallocate(Range(ARG0(registers), ARG1(registers)));

    RETVAL(registers) = ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(WM_COMMAND)
{
    // TODO: move out and implement
    enum class WMCommand {
        CREATE_WINDOW,
        CLOSE_WINDOW,
        POP_EVENT,
        SET_WINDOW_TITLE
    };

    // TODO: Extremely unsafe code, fix

    switch (*Address(ARG0(registers)).as_pointer<WMCommand>()) {
    case WMCommand::CREATE_WINDOW:
    case WMCommand::CLOSE_WINDOW:
    case WMCommand::POP_EVENT:
    case WMCommand::SET_WINDOW_TITLE:
    default:
        RETVAL(registers) = -ErrorCode::INVALID_ARGUMENT;
    }
}

SYSCALL_IMPLEMENTATION(SLEEP)
{
    Thread::current()->set_invulnerable(false);
    sleep::for_nanoseconds(ARG0(registers));

    RETVAL(registers) = ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(DEBUG_LOG)
{
    auto string = StringView(Address(ARG0(registers)).as_pointer<const char>());
    log() << "P[" << Process::current().id() << "][" << Thread::current()->id() << "] log: " << string;

    RETVAL(registers) = ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(MAX)
{
    runtime::panic("Invoked MAX syscall");
}

SYSCALL_IMPLEMENTATION(CREATE_PROCESS)
{
    // TODO
}

SYSCALL_IMPLEMENTATION(CREATE_THREAD)
{
    // TODO
}

}
