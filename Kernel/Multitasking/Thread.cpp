#include "Thread.h"

namespace kernel {

Thread* Thread::s_current;
u32     Thread::s_next_thread_id;
TSS*    Thread::s_tss;

void Thread::initialize()
{
    s_tss = new TSS;
}

RefPtr<Thread> Thread::create_supervisor_thread(Address kernel_stack, Address entrypoint)
{
    Address adjusted_stack = kernel_stack - sizeof(supervisor_thread_stack_frame);

    auto thread                        = new Thread(AddressSpace::of_kernel(), adjusted_stack);
    thread->m_is_supervisor            = true;
    thread->m_initial_kernel_stack_top = kernel_stack;

    auto* frame          = new (adjusted_stack.as_pointer<void>()) supervisor_thread_stack_frame;
    auto& switcher_frame = frame->switcher_frame;
    auto& iret_frame     = frame->iret_frame;

    iret_frame.eflags              = CPU::FLAGS::INTERRUPTS;
    iret_frame.code_selector       = GDT::kernel_code_selector();
    iret_frame.instruction_pointer = entrypoint;

    switcher_frame.instruction_pointer = &supervisor_thread_entrypoint;
    switcher_frame.ebx                 = 0xDEADC0DE;
    switcher_frame.esi                 = 0xDEADBEEF;
    switcher_frame.edi                 = 0xC0DEBEEF;
    switcher_frame.ebp                 = 0x00000000;

    return thread;
}

RefPtr<Thread>
Thread::create_user_thread(AddressSpace& page_dir, Address user_stack, Address kernel_stack, Address entrypoint)
{
    Address adjusted_stack = kernel_stack - sizeof(user_thread_stack_frame);

    auto thread                        = new Thread(page_dir, adjusted_stack);
    thread->m_initial_kernel_stack_top = kernel_stack;

    auto* frame          = new (adjusted_stack.as_pointer<void>()) user_thread_stack_frame;
    auto& iret_frame     = frame->iret_frame;
    auto& switcher_frame = frame->switcher_frame;

    static constexpr auto rpl_ring_3 = 0x3;

    iret_frame.data_selector       = GDT::userland_data_selector() | rpl_ring_3;
    iret_frame.stack_pointer       = user_stack;
    iret_frame.eflags              = CPU::FLAGS::INTERRUPTS;
    iret_frame.code_selector       = GDT::userland_code_selector() | rpl_ring_3;
    iret_frame.instruction_pointer = entrypoint;

    switcher_frame.instruction_pointer = &user_thread_entrypoint;
    switcher_frame.ebx                 = 0xDEADC0DE;
    switcher_frame.esi                 = 0xDEADBEEF;
    switcher_frame.edi                 = 0xC0DEBEEF;
    switcher_frame.ebp                 = 0x00000000;

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
        s_tss->set_kernel_stack_pointer(m_initial_kernel_stack_top);

    m_page_directory.make_active();

    s_current = this;
}

void Thread::deactivate()
{
    if (is_sleeping())
        return;

    m_state = State::READY;
}
}
