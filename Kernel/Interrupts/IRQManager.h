#pragma once

#include "Common/Types.h"

#include "Core/Registers.h"

#define IRQ_HANDLER_SYMBOL(index) extern "C" void irq##index##_handler()

namespace kernel {

class IRQHandler;

class IRQManager {
public:
    static constexpr u16 entry_count    = 16;
    static constexpr u16 irq_base_index = 32;

    static void install();

    static void register_irq_handler(IRQHandler&);
    static void unregister_irq_handler(IRQHandler&);

private:
    static bool has_subscriber(u16 request_number);

    static void irq_handler(RegisterState*) USED;

private:
    static IRQHandler* s_handlers[entry_count];
};
}
