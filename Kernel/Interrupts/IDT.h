#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel {
    class IDT
    {
    public:
        enum attributes : u8
        {
            RING_0         = 0b0,
            RING_3         = SET_BIT(5) | SET_BIT(6),
            TASK_GATE      = SET_BIT(0) | SET_BIT(2),
            INTERRUPT_GATE = SET_BIT(1) | SET_BIT(2) | SET_BIT(3),
            TRAP_GATE      = SET_BIT(0) | SET_BIT(1) | SET_BIT(2) | SET_BIT(3),
            PRESENT        = SET_BIT(7),
        };

        static constexpr u16 entry_count = 256;
        using isr = void(*)();

        IDT& register_isr(u16 index, attributes attrs, isr handler);
        IDT& register_interrupt_handler(u16 index, isr handler);
        IDT& register_user_interrupt_handler(u16 index, isr handler);

        void install();

        static IDT& the();
    private:
        IDT();

        struct PACKED entry
        {
            u16 address_lower;
            u16 selector;
            u8  unused;
            u8  attributes;
            u16 address_higher;
        } m_entries[entry_count];

        struct PACKED pointer
        {
            u16     size;
            entry*  address;
        } m_pointer;

        static IDT s_instance;
    };
}
