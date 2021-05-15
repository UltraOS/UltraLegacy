#pragma once

#include "Common/Types.h"
#include "InterruptController.h"

namespace kernel {

class IOAPIC {
    MAKE_STATIC(IOAPIC);

public:
    static constexpr auto default_register_alignment = 0x10;

    enum class Register : u32 {
        ID = 0,
        VERSION = 1,
        ARBITRATION_ID = 3,
        REDIRECTION_TABLE = 16, // 16 -> however many entries * 2
    };

    static void initialize_all();

    static u32 read_register(Address, Register);
    static void write_register(Address, Register, u32 data);

    static u32 redirection_entry_count(Address);

    static void map_non_msi_pci(const PCIIRQ&, u8 to_index);
    static void map_legacy_irq(const IRQ&, u8 to_index);

    static const IOAPICInfo& responsible_for_gsi(u32);

private:
    static void apply_nmi(const IOAPICInfo&, const IOAPICNMI&);

    static void select_register(Address, Register);

    enum class DeliveryMode : u8 {
        FIXED = 0,
        LOWEST_PRIORITY = 1,
        SMI = 2,
        NMI = 4,
        INIT = 5,
        EXTINT = 7
    };

    enum class DestinationMode : u8 {
        PHYSICAL = 0,
        LOGICAL = 1
    };

    enum class DeliveryStatus : u8 {
        IDLE = 0,
        PENDING = 1
    };

    enum class PinPolarity : u8 {
        ACTIVE_HIGH = 0,
        ACTIVE_LOW = 1
    };

    enum class TriggerMode : u8 {
        EDGE = 0,
        LEVEL = 1
    };

    struct RedirectionEntry {
        u8 index;
        DeliveryMode delivery_mode : 3;
        DestinationMode destination_mode : 1;
        DeliveryStatus delivery_status : 1;
        PinPolarity pin_polarity : 1;
        u8 remote_irr : 1;
        TriggerMode trigger_mode : 1;
        bool is_disabled : 1;
        u64 reserved_1 : 39;
        u8 lapic_id : 4;
        u8 reserved_2 : 4;

        void apply_to(Address, u8 gsi);

        static constexpr size_t size = 8;
    };

    static_assert(sizeof(RedirectionEntry) == RedirectionEntry::size);
};
}
