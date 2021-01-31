#pragma once

#include "Core/CPU.h"
#include "Common/DynamicArray.h"

namespace kernel {

enum class Polarity : u8 {
    CONFORMING = 0,
    ACTIVE_HIGH = 1,
    ACTIVE_LOW = 3
};

enum class TriggerMode : u8 {
    CONFORMING = 0,
    EDGE = 1,
    LEVEL = 3
};

enum class Bus {
    ISA,
    PCI
};

inline Polarity default_polarity_for_bus(Bus bus)
{
    switch (bus) {
    case Bus::ISA:
        return Polarity::ACTIVE_HIGH;
    case Bus::PCI:
        return Polarity::ACTIVE_LOW;
    default:
        return Polarity::CONFORMING;
    }
}

inline TriggerMode default_trigger_mode_for_bus(Bus bus)
{
    switch (bus) {
    case Bus::ISA:
        return TriggerMode::EDGE;
    case Bus::PCI:
        return TriggerMode::LEVEL;
    default:
        return TriggerMode::CONFORMING;
    }
}

struct IRQInfo {
    u32 original_irq_index;
    u32 redirected_irq_index;
    Polarity polarity;
    TriggerMode trigger_mode;
};

struct SMPData {
    DynamicArray<u8> ap_lapic_ids;
    u8 bsp_lapic_id;
    Address lapic_address;
    Address ioapic_address;
    Map<u8, IRQInfo, Less<>> irqs_to_info;
};

inline StringView to_string(Polarity p)
{
    switch (p) {
    case Polarity::CONFORMING:
        return "Conforming"_sv;
    case Polarity::ACTIVE_HIGH:
        return "Active High"_sv;
    case Polarity::ACTIVE_LOW:
        return "Active Low"_sv;
    default:
        return "Unknown"_sv;
    }
}

inline StringView to_string(TriggerMode t)
{
    switch (t) {
    case TriggerMode::CONFORMING:
        return "Conforming"_sv;
    case TriggerMode::EDGE:
        return "Edge"_sv;
    case TriggerMode::LEVEL:
        return "Level"_sv;
    default:
        return "Unknown"_sv;
    }
}

namespace Interrupts {

inline bool are_enabled()
{
    return CPU::flags() & CPU::FLAGS::INTERRUPTS;
}

inline void enable()
{
    sti();
}

inline void disable()
{
    cli();
}

class ScopedDisabler {
public:
    ScopedDisabler()
        : m_should_enable(are_enabled())
    {
        disable();
    }

    ~ScopedDisabler()
    {
        if (m_should_enable)
            enable();
    }

private:
    bool m_should_enable { false };
};

}

}
