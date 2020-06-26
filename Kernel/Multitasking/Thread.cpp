#include "Thread.h"

namespace kernel {

TSS* Thread::s_tss;

void Thread::initialize()
{
    s_tss = new TSS;
}

Thread::Thread(PageDirectory* page_directory, ptr_t kernel_stack_top, ptr_t eip)
    : m_control_block { kernel_stack_top - 20, page_directory }
{
    *reinterpret_cast<u32*>(kernel_stack_top - 4)  = eip;
    *reinterpret_cast<u32*>(kernel_stack_top - 8)  = kernel_stack_top;
    *reinterpret_cast<u32*>(kernel_stack_top - 12) = kernel_stack_top;
    *reinterpret_cast<u32*>(kernel_stack_top - 16) = kernel_stack_top;
    *reinterpret_cast<u32*>(kernel_stack_top - 20) = kernel_stack_top;
}

// TODO: this should be private as it's only an acceptible constructor for the kernel's main thread
Thread::Thread(PageDirectory* page_directory, ptr_t kernel_stack_top)
    : m_control_block { kernel_stack_top, page_directory }
{
}

void Thread::set_next(Thread* thread)
{
    m_next_thread = thread;
}

Thread* Thread::get_next()
{
    return m_next_thread;
}
}
