#include "Core/Logger.h"

#include "InterruptDescriptorTable.h"

namespace kernel {
    InterruptDescriptorTable InterruptDescriptorTable::s_instance;

    InterruptDescriptorTable::InterruptDescriptorTable()
        : m_entries(), m_pointer{entry_count * sizeof(entry) - 1, m_entries}
    {
    }

    InterruptDescriptorTable& InterruptDescriptorTable::the()
    {
        return s_instance;
    }

    InterruptDescriptorTable& InterruptDescriptorTable::register_isr(u16 index, attributes attrs, isr handler)
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

    InterruptDescriptorTable& InterruptDescriptorTable::register_interrupt_handler(u16 index, isr handler)
    {
        return register_isr(index, attributes(INTERRUPT_GATE | RING_3 | PRESENT), handler);
    }

    InterruptDescriptorTable& InterruptDescriptorTable::register_user_interrupt_handler(u16 index, isr handler)
    {
        return register_isr(index, attributes(TRAP_GATE | RING_3 | PRESENT), handler);
    }

    void InterruptDescriptorTable::install()
    {
        asm("lidt %0" ::"m"(m_pointer));
    }
}
