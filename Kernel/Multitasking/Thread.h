#pragma once

#include "TSS.h"
#include "Memory/PageDirectory.h"

namespace kernel {

class Thread {
public:
    struct ControlBlock
    {
        ptr_t          kernel_stack_top;
        PageDirectory* page_directory;
    };

    Thread(PageDirectory* page_directory, ptr_t kernel_stack_top);
    Thread(PageDirectory* page_directory, ptr_t kernel_stack_top, ptr_t eip);
    static void initialize();

    void set_next(Thread*);
    Thread* get_next();

    ControlBlock* control_block() { return &m_control_block; }
private:
    Thread* m_next_thread;

    ControlBlock m_control_block;

    static TSS* s_tss;
};
}
