#include "Core/Runtime.h"

#include "Scheduler.h"

namespace kernel {

Scheduler* Scheduler::s_instance;

void Scheduler::inititalize()
{
    s_instance = new Scheduler();
}

Scheduler& Scheduler::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

ptr_t Scheduler::on_tick(RegisterState)
{
    log() << "Scheduler::on_tick";

    // just return the current esp
    ptr_t current_esp;
    asm("movl %%esp, %%eax" : "=a"(current_esp));
    return current_esp;
}
}
