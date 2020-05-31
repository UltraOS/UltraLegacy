#include "Core/Logger.h"

#include "IDT.h"

namespace kernel {
    IDT IDT::s_instance;

    IDT::IDT()
        : m_entries(), m_pointer{entry_count * sizeof(entry) - 1, m_entries}
    {
    }

    IDT& IDT::the()
    {
        return s_instance;
    }

    IDT& IDT::register_isr(u16 index, attributes attrs, isr handler)
    {
        if (!handler)
        {
            error() << "Someone tried to register an invalid (NULL) isr!";
            hang();
        }

        auto& the_entry = m_entries[index];

        the_entry.address_lower  =  reinterpret_cast<u32>(handler) & 0x0000FFFF;
        the_entry.address_higher = (reinterpret_cast<u32>(handler) & 0xFFFF0000) >> 16;
        the_entry.attributes = attrs;
        the_entry.selector = gdt_selector;

        return *this;
    }

    IDT& IDT::register_interrupt_handler(u16 index, isr handler)
    {
        return register_isr(index, attributes(INTERRUPT_GATE | RING_3 | PRESENT), handler);
    }

    IDT& IDT::register_user_interrupt_handler(u16 index, isr handler)
    {
        return register_isr(index, attributes(TRAP_GATE | RING_3 | PRESENT), handler);
    }

    void IDT::install()
    {
        asm("lidt %0" ::"m"(m_pointer));
    }
}
