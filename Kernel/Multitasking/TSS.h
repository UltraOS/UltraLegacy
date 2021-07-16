#pragma once
#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/GDT.h"

namespace kernel {

class PACKED TSS {
public:
    static constexpr size_t size = 104;

    static constexpr u8 non_maskable_ist_slot = 1;
    static constexpr u8 double_fault_ist_slot = 2;
    static constexpr u8 machine_check_exception_ist_slot = 3;

    TSS();

    void set_kernel_stack_pointer(Address ptr)
    {
        m_kernel_stack_pointer = ptr.as_pointer<u8>();
    }

private:
    u32 m_unused_1;
    u8* m_kernel_stack_pointer { nullptr };
#ifdef ULTRA_32
    u32 m_kernel_stack_segment = GDT::kernel_data_selector();
    u32 m_unused_2[23];
#elif defined(ULTRA_64)
    u32 m_unused_3[6]; // RSP1/RSP2 + IGN
    u64 m_ist_slots[7];
    u32 m_unused_4[3];
#endif
};

static_assert(sizeof(TSS) == TSS::size);
}
