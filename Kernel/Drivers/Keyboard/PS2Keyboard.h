#pragma once

#include "Interrupts/IRQHandler.h"
#include "Keyboard.h"

namespace kernel {

class PS2Keyboard : public Keyboard, public IRQHandler {
public:
    static constexpr u16 irq_number = 1;

    PS2Keyboard();

    void handle_irq(const RegisterState& registers) override;

    StringView name() const override { return "PS2 Keyboard"; }
};
}
