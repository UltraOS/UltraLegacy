#pragma once

#include "Common/DynamicArray.h"
#include "Common/Macros.h"
#include "Common/String.h"
#include "Common/Types.h"
#include "Utilities.h"

namespace kernel {

class InterruptController {
    MAKE_INHERITABLE_SINGLETON(InterruptController) = default;

public:
    enum class Type {
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

    virtual void enable_irq(u8 index) = 0;
    virtual void disable_irq(u8 index) = 0;

    virtual bool is_spurious(u8 request_number) = 0;
    virtual void handle_spurious_irq(u8 request_number) = 0;

    virtual Type type() const = 0;

private:
    static SMPData* s_smp_data;
    static InterruptController* s_instance;
};
}
