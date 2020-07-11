#pragma once

#include "Common/Types.h"
#include "Memory/PageDirectory.h"

namespace kernel {

class LAPIC {
public:
    static constexpr u32 spurious_irq_index = 0xFF;

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
        INTERRUPT_COMMAND_REGISTER_LOWER    = 0x300, // 0x300 -> 0x310
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

private:
    static Address s_base;
};
}
