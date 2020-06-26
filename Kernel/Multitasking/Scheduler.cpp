#include "Core/Runtime.h"

#include "Scheduler.h"

namespace kernel {

Scheduler* Scheduler::s_instance;
Thread*    Scheduler::s_current_thread;

// defined in Core/crt0.asm
extern "C" ptr_t kernel_stack_begin;

void Scheduler::inititalize()
{
    Thread::initialize();
    s_instance       = new Scheduler();
    s_current_thread = new Thread(&PageDirectory::current(), reinterpret_cast<ptr_t>(&kernel_stack_begin));
    the().m_threads  = s_current_thread;
}

Thread* Scheduler::current_thread()
{
    return s_current_thread;
}

void Scheduler::add_task(Thread& thread)
{
    m_threads->set_next(&thread);
}

Scheduler& Scheduler::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

void Scheduler::on_tick(const RegisterState&)
{
    if (!s_current_thread->get_next())
        return;

    auto* old_thread = s_current_thread;

    s_current_thread = s_current_thread->get_next();
    s_current_thread->set_next(old_thread);

    E9Logger e;
    e.write("\n");

    switch_task(old_thread->control_block(), s_current_thread->control_block());
}
}
