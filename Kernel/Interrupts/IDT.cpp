#include "Common/Logger.h"

#include "Core/GDT.h"
#include "IDT.h"
#include "Multitasking/TSS.h"

namespace kernel {

IDT IDT::s_instance;

IDT::IDT()
    : m_entries()
    , m_pointer { entry_count * sizeof(entry) - 1, m_entries }
{
}

IDT& IDT::the()
{
    return s_instance;
}

IDT& IDT::register_isr(u16 index, attributes attrs, isr handler)
{
    if (!handler) {
        String error_string;
        error_string << "IDT: Tried to register an invalid (NULL) isr at index " << index;
        runtime::panic(error_string.data());
    }

    auto& the_entry = m_entries[index];

    the_entry.address_1 = reinterpret_cast<size_t>(handler) & 0x0000FFFF;
    the_entry.address_2 = (reinterpret_cast<size_t>(handler) & 0xFFFF0000) >> 16;
#ifdef ULTRA_64
    the_entry.address_3 = (reinterpret_cast<size_t>(handler) & 0xFFFFFFFF00000000) >> 32;
#endif
    the_entry.attributes = attrs;
    the_entry.selector = GDT::kernel_code_selector();

    return *this;
}

IDT& IDT::register_interrupt_handler(u16 index, isr handler)
{
    return register_isr(index, INTERRUPT_GATE | RING_0 | PRESENT, handler);
}

IDT& IDT::register_user_interrupt_handler(u16 index, isr handler)
{
    return register_isr(index, TRAP_GATE | RING_3 | PRESENT, handler);
}

#ifdef ULTRA_64
void IDT::configure_ist()
{
    static constexpr auto nmi_index = 0x2;
    static constexpr auto df_index = 0x8;
    static constexpr auto pf_index = 0xE;
    static constexpr auto mce_index = 0x12;

    m_entries[nmi_index].ist_slot = TSS::non_maskable_ist_slot;
    m_entries[df_index].ist_slot = TSS::double_fault_ist_slot;
    m_entries[pf_index].ist_slot = TSS::page_fault_ist_slot;
    m_entries[mce_index].ist_slot = TSS::machine_check_expection_ist_slot;

    install();
}
#endif

void IDT::install()
{
    asm("lidt %0" ::"m"(m_pointer));
}
}
