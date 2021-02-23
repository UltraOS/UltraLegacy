#pragma once

#include "InterruptController.h"

namespace kernel {

class APIC : public InterruptController {
    MAKE_SINGLETON_INHERITABLE(InterruptController, APIC);

public:
    void end_of_interrupt(u8 request_number) override;

    void clear_all() override;

    void enable_irq_for(const IRQHandler&) override;
    void disable_irq_for(const IRQHandler&) override;

    StringView device_name() const override { return "APIC"_sv; }
    Model model() const override { return Model::APIC; }
};
}
