#pragma once

#include "Common/DynamicArray.h"
#include "Common/Optional.h"
#include "Core/CPU.h"

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

enum class Bus : u8 {
    ISA = 0,
    PCI = 1,
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

struct IRQ {
    u32 irq_index;
    u32 gsi;
    Polarity polarity;
    TriggerMode trigger_mode;
};

struct LAPICInfo {
    struct NMI {
        Polarity polarity;
        TriggerMode trigger_mode;
        u8 lint;
    };

    u8 id;
    u8 acpi_uid;
    Optional<NMI> nmi_connection;
};

struct IOAPICNMI {
    Polarity polarity;
    TriggerMode trigger_mode;
    u32 gsi;
};

struct IOAPICInfo {
    using GSIRange = BasicRange<u32>;
    u8 id;
    GSIRange gsi_range;
    Address address;

    u32 rebase_gsi(u32 gsi) const
    {
        ASSERT(gsi_range.contains(gsi));
        return gsi - gsi_range.begin();
    }
};

struct SMPData {
    DynamicArray<LAPICInfo> lapics;
    Address lapic_address;
    u8 bsp_lapic_id;

    DynamicArray<IOAPICNMI> ioapic_nmis;
    DynamicArray<IOAPICInfo> ioapics;

    Map<u16, IRQ, Less<>> legacy_irqs_to_info;
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

inline static constexpr size_t legacy_irq_base = 32;
inline static constexpr size_t legacy_irq_count = 16;

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
