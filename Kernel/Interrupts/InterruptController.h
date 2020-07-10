#pragma once

#include "Common/Types.h"

namespace kernel {

class InterruptController {
public:
    static void discover_and_setup();

    static InterruptController& the();

    virtual void end_of_interrupt(u8 request_number) = 0;

    virtual void clear_all() = 0;

    virtual void enable_irq(u8 index)  = 0;
    virtual void disable_irq(u8 index) = 0;

    virtual bool is_spurious(u8 request_number)         = 0;
    virtual void handle_spurious_irq(u8 request_number) = 0;

private:
    static InterruptController* s_instance;
};
}
