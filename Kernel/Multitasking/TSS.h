#pragma once
#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/GDT.h"

namespace kernel {

class TSS {
public:
    static constexpr size_t size = 64;

    TSS()
    {
        GDT::the().create_tss_descriptor(this);
        GDT::the().install();
    }

    void set_kernel_stack_pointer(Address ptr) { m_kernel_stack_pointer = ptr; }

private:
    // unused since we don't do hardware switching
    u32 m_previous_tss { 0 };

    Address m_kernel_stack_pointer { nullptr };
    u32     m_kernel_stack_segment = GDT::kernel_data_selector();
    u8      m_unused[52] {};
};

static_assert(sizeof(TSS) == TSS::size);
}
