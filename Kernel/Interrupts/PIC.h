#pragma once

#include "Common/Types.h"
#include "IRQHandler.h"
#include "InterruptController.h"

namespace kernel {

class PIC : public InterruptController {
    MAKE_SINGLETON_INHERITABLE(InterruptController, PIC);

public:
    static constexpr u16 max_irq_index = 15;
    static constexpr u8 slave_irq_index = 2;

    static constexpr u16 spurious_master = 7;
    static constexpr u16 spurious_slave = 15;

    static constexpr u8 master_port = 0x20;
    static constexpr u8 slave_port = 0xA0;

    static constexpr u8 master_command = master_port;
    static constexpr u8 master_data = master_port + 1;

    static constexpr u8 slave_command = slave_port;
    static constexpr u8 slave_data = slave_port + 1;

    static constexpr u8 end_of_interrupt_code = 0x20;

    void end_of_interrupt(u8 request_number) override;

    void clear_all() override;

    void enable_irq(u8 index) override;
    void disable_irq(u8 index) override;

    bool is_spurious(u8 request_number) override;
    void handle_spurious_irq(u8 request_number) override;

    static void ensure_disabled();

private:
    bool is_irq_being_serviced(u8 request_number);
    void remap(u8 offset);
    void set_raw_irq_mask(u8 mask, bool master);
};
}
