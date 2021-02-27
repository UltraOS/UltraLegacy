#pragma once

#include "InterruptController.h"

namespace kernel {

class APIC : public InterruptController {
public:
    APIC();

    void end_of_interrupt(u8 request_number) override;

    void clear_all() override;

    void enable_irq_for(const IRQHandler&) override;
    void disable_irq_for(const IRQHandler&) override;

    static constexpr StringView type = "APIC"_sv;
    StringView device_type() const override { return type; }
    StringView device_model() const override { return "xAPIC"_sv; }
};
}
