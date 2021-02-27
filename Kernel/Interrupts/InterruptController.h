#pragma once

#include "Common/DynamicArray.h"
#include "Common/Macros.h"
#include "Common/String.h"
#include "Common/Types.h"
#include "Drivers/Device.h"
#include "Utilities.h"

namespace kernel {

class IRQHandler;

class InterruptController : public Device {
public:
    InterruptController();

    static void discover_and_setup();

    static InterruptController& primary();

    static bool is_legacy_mode() { return s_smp_data == nullptr; }

    static SMPData& smp_data()
    {
        ASSERT(s_smp_data != nullptr);

        return *s_smp_data;
    }

    static bool is_initialized();

    virtual void end_of_interrupt(u8 request_number) = 0;

    virtual void clear_all() = 0;

    virtual void enable_irq_for(const IRQHandler&) = 0;
    virtual void disable_irq_for(const IRQHandler&) = 0;

private:
    static SMPData* s_smp_data;
};
}
