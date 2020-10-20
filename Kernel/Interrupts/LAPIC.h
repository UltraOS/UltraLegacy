#pragma once

#include "Common/Types.h"
#include "IRQHandler.h"
#include "Memory/AddressSpace.h"

namespace kernel {

class LAPIC {
    MAKE_STATIC(LAPIC);

public:
    static constexpr u32 invalid_destination = -1;
    static constexpr u32 spurious_irq_index  = 0xFF;

    class Timer : IRQHandler {
        friend class LAPIC;
        MAKE_SINGLETON(Timer) : IRQHandler(irq_number) { }

    public:
        static constexpr u32 irq_number       = 253;
        static constexpr u32 masked_bit       = 1 << 16;
        static constexpr u32 periodic_mode    = 1 << 17;
        static constexpr u32 ticks_per_second = 100;

        void initialize_for_this_processor();

        void handle_irq(const RegisterState& registers) override;
        void finalize_irq() override { }

        void enable_irq() override;
        void disable_irq() override;
    };

public:
    static Timer& timer()
    {
        ASSERT(s_timer != nullptr);

        return *s_timer;
    }

    enum class Register {
        ID                                  = 0x20,
        VERSION                             = 0x30,
        TASK_PRIORITY                       = 0x80,
        ARBITRATION_PRIORITY                = 0x90,
        PROCESS_PRIORITY                    = 0xA0,
        END_OF_INTERRUPT                    = 0xB0,
        REMOTE_READ                         = 0xC0,
        LOGICAL_DESTINATION                 = 0xD0,
        DESTINATION_FORMAT                  = 0xE0,
        SPURIOUS_INTERRUPT_VECTOR           = 0xF0,
        IN_SERVICE                          = 0x100, // 0x100 -> 0x170
        TRIGGER_MODE                        = 0x180, // 0x180 -> 0x1F0
        INTERRUPT_REQUEST                   = 0x200, // 0x200 -> 0x270
        ERROR_STATUS                        = 0x280,
        CORRECTED_MACHING_CHECK_INTERRUPT   = 0x2F0,
        INTERRUPT_COMMAND_REGISTER_LOWER    = 0x300,
        INTERRUPT_COMMAND_REGISTER_HIGHER   = 0x310,
        LVT_TIMER                           = 0x320,
        LVT_THERMAL_SENSOR                  = 0x330,
        LVT_PERFORMANCE_MONITORING_COUNTERS = 0x340,
        LVT_LOCAL_INTERRUPT_0               = 0x350,
        LVT_LOCAL_INTERRUPT_1               = 0x360,
        LVT_ERROR                           = 0x370,
        INITIAL_COUNT                       = 0x380,
        CURRENT_COUNT                       = 0x390,
        DIVIDE_CONFIGURATION                = 0x3E0
    };

    static void set_base_address(Address physical_address);

    static void initialize_for_this_processor();

    static void write_register(Register, u32 value);
    static u32  read_register(Register);

    static void end_of_interrupt();

    static void start_processor(u8);

    static u32 my_id();

    enum class DestinationType : u8 { SPECIFIC, SELF, ALL_INCLUDING_SELF, ALL_EXCLUDING_SELF };

    template <DestinationType dt>
    static enable_if_t<dt == DestinationType::SPECIFIC> send_ipi(u32 destination_id)
    {
        send_ipi(dt, destination_id);
    }

    template <DestinationType dt>
    static enable_if_t<dt == DestinationType::SELF || dt == DestinationType::ALL_INCLUDING_SELF
                       || dt == DestinationType::ALL_EXCLUDING_SELF>
    send_ipi()
    {
        send_ipi(dt);
    }

private:
    enum class DeliveryMode : u8 { NORMAL = 0, LOWEST_PRIORITY = 1, SMI = 2, NMI = 4, INIT = 5, SIPI = 6 };

    enum class DestinationMode : u8 { PHYSICAL, LOGICAL };

    enum class Level : u8 { DE_ASSERT, ASSERT };

    enum class TriggerMode : u8 { EDGE, LEVEL };

    struct ICR {
        // ICR LOWER
        union {
            u8 vector_number;
            u8 entrypoint_page;
        };
        DeliveryMode    delivery_mode : 3;
        DestinationMode destination_mode : 1;
        bool            is_pending : 1;
        u8              reserved_1 : 1;
        Level           level : 1;
        TriggerMode     trigger_mode : 1;
        u8              reserved_2 : 2;
        DestinationType destination_type : 2;
        u16             reserved_3 : 12;

        // ICR HIGHER
        u32 reserved_4 : 24;
        u8  destination_id;
    };

    static constexpr size_t icr_size = 8;

    static_assert(sizeof(ICR) == icr_size);

    static void send_ipi(DestinationType, u32 = invalid_destination);
    static void send_init_to(u8);
    static void send_startup_to(u8);

private:
    static Address s_base;
    static Timer*  s_timer;
};
}
