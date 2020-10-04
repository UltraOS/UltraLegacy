#include "Common/Logger.h"

#include "Core/GDT.h"
#include "IDT.h"

namespace kernel {

IDT IDT::s_instance;

IDT::IDT() : m_entries(), m_pointer { entry_count * sizeof(entry) - 1, m_entries } { }

IDT& IDT::the()
{
    return s_instance;
}

IDT& IDT::register_isr(u16 index, attributes attrs, isr handler)
{
    if (!handler) {
        StackStringBuilder error_string;
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
    the_entry.selector   = GDT::kernel_code_selector();

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

void IDT::install()
{
    asm("lidt %0" ::"m"(m_pointer));
}
}
