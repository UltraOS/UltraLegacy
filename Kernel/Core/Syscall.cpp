#include "Common/Logger.h"

#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"

#include "Syscall.h"

namespace kernel {

void Syscall::exit(size_t code)
{
    Scheduler::the().exit(code);
}

void Syscall::debug_log(const char* string)
{
    log() << "DebugLog: " << string;
}
}
