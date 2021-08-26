#include "Core/FPU.h"
#include "Core/Registers.h"

#include "Interrupts/IDT.h"
#include "Process.h"
#include "Scheduler.h"
#include "TaskFinalizer.h"
#include "Thread.h"

namespace kernel {

AddressSpace& Thread::address_space()
{
    return m_owner.address_space();
}

RefPtr<Thread> Thread::create_idle(Process& owner)
{
    auto* thread = new Thread(owner);

    // I don't like the fact that create_idle makes many assumptions about the purpose
    // of the idle thread as well as its state. Here we assume that the cpu is already
    // running this thread to kickstart the scheduler properly. I also don't like the fact
    // that we don't know the actual initial stack value of the thread. While it's not very
    // useful right now it could be necessary later. Maybe I should refactor this later.
    thread->m_state = State::RUNNING;
    CPU::current().set_current_thread(thread);

    return thread;
}

RefPtr<Thread> Thread::create_supervisor(Process& owner, RefPtr<VirtualRegion> kernel_stack, Address entrypoint)
{
    Address adjusted_stack = kernel_stack->virtual_range().end() - sizeof(RegisterState);
#ifdef ULTRA_32
    adjusted_stack += sizeof(u32) * 2; // ss and esp are not popped
#endif
    // force misalign the stack to sizeof(void*) for sysv abi
    adjusted_stack -= sizeof(ptr_t);

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
    frame.rsp = kernel_stack->virtual_range().end();
    frame.rip = entrypoint;
#endif

    return thread;
}

RefPtr<Thread> Thread::create_user(
    Process& owner,
    RefPtr<VirtualRegion> kernel_stack,
    TaskLoader::LoadRequest* load_request)
{
    Address adjusted_stack = kernel_stack->virtual_range().end() - sizeof(RegisterState) * 2;

    // Layout of a fresh kernel stack (first is popped last):
    // ------ i386 -----
    // - Userspace iret frame, instruction pointer is set later by TaskLoader after loading the binary.
    // - dummy u32 (SysV abi)
    // - dummy u32 (SysV abi)
    // - dummy u32 (SysV abi)
    // - LoadRequest pointer
    // - dummy ret address
    // - Kernel iret frame, instruction pointer set to TaskLoader::do_load
    // ----- AMD64 -----
    // - Userspace iret frame, same as above
    // - dummy ret address
    // - Kernel iret frame, same as above
#ifdef ULTRA_64
    adjusted_stack -= sizeof(u64);
#elif defined(ULTRA_32)
    adjusted_stack -= sizeof(u32) * 5;
    adjusted_stack += sizeof(u32) * 2; // ss and esp are not popped for kernel iret
#endif
    auto thread = new Thread(owner, kernel_stack, IsSupervisor::NO);
    thread->m_control_block.current_kernel_stack_top = adjusted_stack.raw();

    auto* frame = new (adjusted_stack.as_pointer<void>()) RegisterState {};

#ifdef ULTRA_32
    frame->ss = GDT::kernel_data_selector();
    frame->gs = GDT::kernel_data_selector();
    frame->fs = GDT::kernel_data_selector();
    frame->es = GDT::kernel_data_selector();
    frame->ds = GDT::kernel_data_selector();

    frame->eip = Address(&TaskLoader::do_load);
    frame->cs = GDT::kernel_code_selector();
    frame->eflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);

    // skip kernel iret
    adjusted_stack += sizeof(RegisterState) - (2 * sizeof(u32));

    // skip fake ret address
    adjusted_stack += sizeof(u32);

    // push load request on the stack
    *adjusted_stack.as_pointer<ptr_t>() = Address(load_request);

    // skip load_request and alignment
    adjusted_stack += sizeof(u32) * 4;
#elif defined(ULTRA_64)
    frame->cs = GDT::kernel_code_selector();
    frame->ss = GDT::kernel_data_selector();
    frame->rflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
    frame->rsp = kernel_stack->virtual_range().end() - sizeof(RegisterState);
    frame->rip = Address(&TaskLoader::do_load);
    frame->rdi = Address(load_request);

    // skip kernel iret + fake return address
    adjusted_stack += sizeof(RegisterState) + sizeof(ptr_t);
#endif

    frame = new (adjusted_stack.as_pointer<void>()) RegisterState {};

    static constexpr auto rpl_ring_3 = 0x3;

#ifdef ULTRA_32
    frame->gs = GDT::userland_data_selector() | rpl_ring_3;
    frame->fs = GDT::userland_data_selector() | rpl_ring_3;
    frame->es = GDT::userland_data_selector() | rpl_ring_3;
    frame->ds = GDT::userland_data_selector() | rpl_ring_3;
    frame->cs = GDT::userland_code_selector() | rpl_ring_3;

    frame->userspace_ss = GDT::userland_data_selector() | rpl_ring_3;
    frame->eflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
#elif defined(ULTRA_64)
    frame->cs = GDT::userland_code_selector() | rpl_ring_3;
    frame->ss = GDT::userland_data_selector() | rpl_ring_3;

    frame->rflags = static_cast<size_t>(CPU::FLAGS::INTERRUPTS);
#endif

    thread->m_fpu_state = FPU::allocate_state();

    return thread;
}

Thread::Thread(Process& owner)
    : m_id(owner.consume_thread_id())
    , m_owner(owner)
    , m_is_supervisor(IsSupervisor::YES)
{
}

Thread::Thread(Process& owner, RefPtr<VirtualRegion> kernel_stack, IsSupervisor is_supervisor)
    : m_id(owner.consume_thread_id())
    , m_owner(owner)
    , m_kernel_stack(kernel_stack)
    , m_control_block { kernel_stack->virtual_range().end() }
    , m_is_supervisor(is_supervisor)
{
}

bool Thread::is_main() const
{
    return m_id == Process::main_thread_id;
}

void Thread::activate()
{
    ASSERT(m_state == State::READY);

    m_state = State::RUNNING;

    if (is_supervisor() == IsSupervisor::NO)
        CPU::current().tss().set_kernel_stack_pointer(m_kernel_stack->virtual_range().end());

    if (m_fpu_state)
        FPU::restore_state(m_fpu_state);

    if (m_owner.address_space() != AddressSpace::current())
        m_owner.address_space().make_active();

    CPU::current().set_current_thread(this);
}

void Thread::deactivate()
{
    if (m_fpu_state)
        FPU::save_state(m_fpu_state);

    if (!is_running())
        return;

    m_state = State::READY;
}

void Thread::unblock()
{
    ASSERT(m_blocker != nullptr);

    if (m_blocker->should_block())
        m_blocker->set_result(Blocker::Result::UNBLOCKED);

    m_blocker = nullptr;

    if (is_blocked())
        m_state = State::READY;
}

bool Thread::interrupt()
{
    ASSERT(m_blocker != nullptr);

    if (!m_blocker->is_interruptable())
        return false;

    m_blocker->set_result(Blocker::Result::INTERRUPTED);
    m_blocker = nullptr;
    m_state = State::READY;

    return true;
}

}
