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
    MAKE_INHERITABLE_SINGLETON(InterruptController) = default;

public:
    enum class Model {
        PIC,
        APIC,
    };

    static void discover_and_setup();

    static InterruptController& the();

    static bool is_legacy_mode() { return s_smp_data == nullptr; }

    static SMPData& smp_data()
    {
        ASSERT(s_smp_data != nullptr);

        return *s_smp_data;
    }

    static bool is_initialized() { return s_instance != nullptr; }

    virtual void end_of_interrupt(u8 request_number) = 0;

    virtual void clear_all() = 0;

    virtual void enable_irq_for(const IRQHandler&) = 0;
    virtual void disable_irq_for(const IRQHandler&) = 0;

    Device::Type device_type() const override { return Device::Type::INTERRUPT_CONTROLLER; }
    virtual Model model() const = 0;

private:
    static SMPData* s_smp_data;
    static InterruptController* s_instance;
};
}
