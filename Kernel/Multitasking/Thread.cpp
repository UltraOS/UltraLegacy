#include "Core/Registers.h"

#include "Thread.h"

namespace kernel {

u32 Thread::s_next_thread_id;

void Thread::initialize()
{
    CPU::current().set_tss(new TSS);
}

RefPtr<Thread> Thread::create_supervisor_thread(Address kernel_stack, Address entrypoint)
{
    Address adjusted_stack = kernel_stack - sizeof(RegisterState)
#ifdef ULTRA_32 // clang-format off
        + sizeof(u32) * 2; // esp and ss are not popped
#elif defined(ULTRA_64)
        ;
#endif // clang-format on

    auto thread                        = new Thread(AddressSpace::of_kernel(), adjusted_stack);
    thread->m_is_supervisor            = true;
    thread->m_initial_kernel_stack_top = kernel_stack;

    auto& frame = *new (adjusted_stack.as_pointer<void>()) RegisterState;

#ifdef ULTRA_32
    frame.ss = GDT::kernel_data_selector();
    frame.gs = GDT::kernel_data_selector();
    frame.fs = GDT::kernel_data_selector();
    frame.es = GDT::kernel_data_selector();
    frame.ds = GDT::kernel_data_selector();

    frame.eip    = entrypoint;
    frame.cs     = GDT::kernel_code_selector();
    frame.eflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
#elif defined(ULTRA_64)
    frame.cs     = GDT::kernel_code_selector();
    frame.ss     = GDT::kernel_data_selector();
    frame.rflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
    frame.rsp    = kernel_stack;
    frame.rip    = entrypoint;
#endif

    return thread;
}

RefPtr<Thread>
Thread::create_user_thread(AddressSpace& page_dir, Address user_stack, Address kernel_stack, Address entrypoint)
{
    Address adjusted_stack = kernel_stack - sizeof(RegisterState);

    auto thread                        = new Thread(page_dir, adjusted_stack);
    thread->m_is_supervisor            = false;
    thread->m_initial_kernel_stack_top = kernel_stack;

    auto& frame = *new (adjusted_stack.as_pointer<void>()) RegisterState;

    static constexpr auto rpl_ring_3 = 0x3;

#ifdef ULTRA_32
    frame.gs           = GDT::userland_data_selector() | rpl_ring_3;
    frame.fs           = GDT::userland_data_selector() | rpl_ring_3;
    frame.es           = GDT::userland_data_selector() | rpl_ring_3;
    frame.ds           = GDT::userland_data_selector() | rpl_ring_3;
    frame.cs           = GDT::userland_code_selector() | rpl_ring_3;
    frame.userspace_ss = GDT::userland_data_selector() | rpl_ring_3;

    frame.userspace_esp = user_stack;

    frame.eip = entrypoint;

    frame.eflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
#elif defined(ULTRA_64)
    frame.cs     = GDT::userland_code_selector() | rpl_ring_3;
    frame.ss     = GDT::userland_data_selector() | rpl_ring_3;
    frame.rflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
    frame.rsp    = user_stack;
    frame.rip    = entrypoint;
#endif

    return thread;
}

Thread::Thread(AddressSpace& page_dir, Address kernel_stack)
    : m_thread_id(s_next_thread_id++), m_page_directory(page_dir), m_control_block { kernel_stack }
{
}

void Thread::activate()
{
    m_state = State::RUNNING;

    if (is_user())
        CPU::current().tss()->set_kernel_stack_pointer(m_initial_kernel_stack_top);

    m_page_directory.make_active();

    CPU::current().set_current_thread(this);
}

void Thread::deactivate()
{
    if (is_sleeping())
        return;

    m_state = State::READY;
}
}
