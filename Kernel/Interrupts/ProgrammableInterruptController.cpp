#include "Core/IO.h"
#include "Core/Logger.h"

#include "ProgrammableInterruptController.h"


namespace kernel {
    void ProgrammableInterruptController::end_of_interrupt(u8 request_number, bool spurious)
    {
        if (request_number >= 8 && !spurious)
            IO::out8<slave_command>(end_of_interrupt_code);

        IO::out8<master_command>(end_of_interrupt_code);
    }

    bool ProgrammableInterruptController::is_irq_being_serviced(u8 request_number)
    {
        static constexpr u8 read_isr = 0x0b;

        IO::out8<master_command>(read_isr);
        IO::out8<slave_command>(read_isr);

        u16 isr_mask = (IO::in8<slave_command>() << 8) | IO::in8<master_command>();

        info() << "PIC: ISR mask=" << isr_mask << " request number=" << request_number;

        return !(isr_mask & request_number);
    }

    void ProgrammableInterruptController::remap(u8 offset)
    {
        u8 master_mask = IO::in8<master_data>();
        u8 slave_mask  = IO::in8<slave_data>();

        static constexpr u8 icw1_icw4 = 0x01;
        static constexpr u8 icw1_init = 0x10;
        static constexpr u8 icw4_8086 = 0x01;

        static constexpr u8 irqs_per_controller = 8;

        static constexpr u8 slave_irq              = 0b00000100;
        static constexpr u8 slave_cascade_identity = 0b00000010;

        IO::out8<master_command>(icw1_init | icw1_icw4);
        IO::wait();

        IO::out8<slave_command>(icw1_init | icw1_icw4);
        IO::wait();

        IO::out8<master_data>(offset);
        IO::wait();

        IO::out8<slave_data>(offset + irqs_per_controller);
        IO::wait();

        IO::out8<master_data>(slave_irq);
        IO::wait();

        IO::out8<slave_data>(slave_cascade_identity);
        IO::wait();

        IO::out8<master_data>(icw4_8086);
        IO::wait();

        IO::out8<slave_data>(icw4_8086);
        IO::wait();

        IO::out8<master_data>(master_mask);
        IO::out8<slave_data>(slave_mask);
    }
}
