#include "TSS.h"
#include "Memory/MemoryManager.h"

namespace kernel {

TSS::TSS()
{
#ifdef ULTRA_64
    auto nmi_ist = MemoryManager::the().allocate_kernel_stack("non-maskable IST"_sv);
    nmi_ist->make_eternal();

    auto df_ist = MemoryManager::the().allocate_kernel_stack("double fault IST"_sv);
    df_ist->make_eternal();

    auto pf_ist = MemoryManager::the().allocate_kernel_stack("page fault IST"_sv);
    pf_ist->make_eternal();

    auto mce_ist = MemoryManager::the().allocate_kernel_stack("machine-check IST"_sv);
    mce_ist->make_eternal();

    m_ist_slots[non_maskable_ist_slot - 1] = nmi_ist->virtual_range().end();
    m_ist_slots[double_fault_ist_slot - 1] = df_ist->virtual_range().end();
    m_ist_slots[page_fault_ist_slot - 1] = pf_ist->virtual_range().end();
    m_ist_slots[machine_check_expection_ist_slot - 1] = mce_ist->virtual_range().end();
#endif

    GDT::the().create_tss_descriptor(*this);
}

}
