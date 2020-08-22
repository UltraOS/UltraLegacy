#include "Common/Logger.h"

#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"

#include "Syscall.h"

namespace kernel {

void Syscall::exit(u8 code)
{
    log() << "Thread " << Thread::current() << " exited with code " << code;

    Interrupts::ScopedDisabler d;
    Thread::current()->exit(code);
    Scheduler::yield();
}

void Syscall::debug_log(const char* string)
{
    log() << "DebugLog: " << string;
}
}
