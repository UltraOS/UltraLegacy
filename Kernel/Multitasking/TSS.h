#pragma once
#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/GDT.h"

namespace kernel {

class PACKED TSS {
public:
    static constexpr size_t size = 104;

    TSS() { GDT::the().create_tss_descriptor(this); }

    void set_kernel_stack_pointer(Address ptr) { m_kernel_stack_pointer = ptr.as_pointer<u8>(); }

private:
#ifdef ULTRA_32
    // unused since we don't do hardware switching
    u32 m_unused_1;
    u8* m_kernel_stack_pointer { nullptr };
    u32 m_kernel_stack_segment = GDT::kernel_data_selector();
#elif defined(ULTRA_64)
    u32 m_unused_1;
    u8* m_kernel_stack_pointer { nullptr };
#endif
    u32 m_unused_2[23];
};

static_assert(sizeof(TSS) == TSS::size);
}
