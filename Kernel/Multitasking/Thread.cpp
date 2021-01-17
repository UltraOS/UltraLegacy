#include "Core/Registers.h"

#include "Interrupts/IDT.h"
#include "Process.h"
#include "Thread.h"

namespace kernel {

AddressSpace& Thread::address_space()
{
    return m_owner.address_space();
}

RefPtr<Thread> Thread::create_idle(Process& owner)
{
    // We can afford to pass a random value as kernel stack
    // because it will be properly filled in once the idle task gets interrupted
    auto* thread = new Thread(owner, Address(0xDEADCAFE), IsSupervisor::YES);

    // I don't like the fact that create_idle makes many assumptions about the purpose
    // of the idle thread as well as its state. Here we assume that the cpu is already
    // running this thread to kickstart the scheduler properly. I also don't like the fact
    // that we don't know the actual initial stack value of the thread. While it's not very
    // useful right now it could be necessary later. Maybe I should refactor this later.
    thread->m_state = State::RUNNING;
    CPU::current().set_current_thread(thread);

    return thread;
}

RefPtr<Thread> Thread::create_supervisor(Process& owner, Address kernel_stack, Address entrypoint)
{
    Address adjusted_stack = kernel_stack - sizeof(RegisterState)
#ifdef ULTRA_32
        + sizeof(u32) * 2; // esp and ss are not popped
#elif defined(ULTRA_64)
        ;
#endif

    auto thread = new Thread(owner, kernel_stack, IsSupervisor::YES);
    thread->m_control_block.current_kernel_stack_top = adjusted_stack.raw();

    auto& frame = *new (adjusted_stack.as_pointer<void>()) RegisterState;

#ifdef ULTRA_32
    frame.ss = GDT::kernel_data_selector();
    frame.gs = GDT::kernel_data_selector();
    frame.fs = GDT::kernel_data_selector();
    frame.es = GDT::kernel_data_selector();
    frame.ds = GDT::kernel_data_selector();

    frame.eip = entrypoint;
    frame.cs = GDT::kernel_code_selector();
    frame.eflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
#elif defined(ULTRA_64)
    frame.cs = GDT::kernel_code_selector();
    frame.ss = GDT::kernel_data_selector();
    frame.rflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
    frame.rsp = kernel_stack;
    frame.rip = entrypoint;
#endif

    return thread;
}

RefPtr<Thread> Thread::create_user(Process& owner, Address kernel_stack, Address user_stack, Address entrypoint)
{
    Address adjusted_stack = kernel_stack - sizeof(RegisterState);

    auto thread = new Thread(owner, adjusted_stack, IsSupervisor::NO);
    thread->m_control_block.current_kernel_stack_top = adjusted_stack;

    auto& frame = *new (adjusted_stack.as_pointer<void>()) RegisterState;

    static constexpr auto rpl_ring_3 = 0x3;

#ifdef ULTRA_32
    frame.gs = GDT::userland_data_selector() | rpl_ring_3;
    frame.fs = GDT::userland_data_selector() | rpl_ring_3;
    frame.es = GDT::userland_data_selector() | rpl_ring_3;
    frame.ds = GDT::userland_data_selector() | rpl_ring_3;
    frame.cs = GDT::userland_code_selector() | rpl_ring_3;

    frame.userspace_ss = GDT::userland_data_selector() | rpl_ring_3;
    frame.userspace_esp = user_stack;
    frame.eip = entrypoint;
    frame.eflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
#elif defined(ULTRA_64)
    frame.cs = GDT::userland_code_selector() | rpl_ring_3;
    frame.ss = GDT::userland_data_selector() | rpl_ring_3;

    frame.rflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
    frame.rsp = user_stack;
    frame.rip = entrypoint;
#endif

    return thread;
}

Thread::Thread(Process& owner, Address kernel_stack, IsSupervisor is_supervisor)
    : m_id(owner.consume_thread_id())
    , m_owner(owner)
    , m_initial_kernel_stack_top(kernel_stack)
    , m_control_block { kernel_stack.raw() }
    , m_is_supervisor(is_supervisor)
{
}

bool Thread::is_main() const
{
    return m_id == Process::main_thread_id;
}

void Thread::activate()
{
    m_state = State::RUNNING;

    if (is_supervisor() == IsSupervisor::NO)
        CPU::current().tss().set_kernel_stack_pointer(m_initial_kernel_stack_top);

    if (m_owner.address_space() != AddressSpace::current())
        m_owner.address_space().make_active();

    CPU::current().set_current_thread(this);
}

void Thread::deactivate()
{
    if (is_sleeping())
        return;

    m_state = State::READY;
}
}
