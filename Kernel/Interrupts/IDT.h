#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class IDT {
public:
    enum attributes : u8 {
        RING_0         = 0b0,
        RING_3         = SET_BIT(5) | SET_BIT(6),
        TASK_GATE      = SET_BIT(0) | SET_BIT(2),
        INTERRUPT_GATE = SET_BIT(1) | SET_BIT(2) | SET_BIT(3),
        TRAP_GATE      = SET_BIT(0) | SET_BIT(1) | SET_BIT(2) | SET_BIT(3),
        PRESENT        = SET_BIT(7),
    };

    friend attributes operator|(attributes l, attributes r)
    {
        return static_cast<attributes>(static_cast<u8>(l) | static_cast<u8>(r));
    }

    static constexpr u16 entry_count = 256;
    using isr                        = void (*)();

    IDT& register_isr(u16 index, attributes attrs, isr handler);
    IDT& register_interrupt_handler(u16 index, isr handler);
    IDT& register_user_interrupt_handler(u16 index, isr handler);

    void install();

    static IDT& the();

private:
    IDT();

    struct PACKED entry {
        u16 address_1;
        u16 selector;
        u8  unused;
        u8  attributes;
        u16 address_2;

#ifdef ULTRA_64
        u32 address_3;
        u32 reserved;
#endif
    } m_entries[entry_count];

    struct PACKED pointer {
        u16    size;
        entry* address;
    } m_pointer;

    static IDT s_instance;
};
}
