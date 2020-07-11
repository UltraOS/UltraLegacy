#pragma once

#include "InterruptController.h"

namespace kernel {

class APIC : public InterruptController {
public:
    APIC();

    void end_of_interrupt(u8 request_number) override;

    void clear_all() override;

    void enable_irq(u8 index) override;
    void disable_irq(u8 index) override;

    bool is_spurious(u8 request_number) override;
    void handle_spurious_irq(u8 request_number) override;
};
}
