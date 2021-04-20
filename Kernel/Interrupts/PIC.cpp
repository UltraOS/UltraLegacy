#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Core/IO.h"

#include "InterruptController.h"
#include "PIC.h"

namespace kernel {

PIC::PIC()
{
    new SpuriousHandler(true);
    new SpuriousHandler(false);

    remap(legacy_irq_base + 1);
    clear_all();
}

PIC::SpuriousHandler::SpuriousHandler(bool master)
    : IRQHandler(IRQHandler::Type::LEGACY, master ? spurious_master : spurious_slave)
{
}

bool PIC::SpuriousHandler::handle_irq(RegisterState&)
{
    if (is_irq_being_serviced(legacy_irq_number()))
        return false;

    if (legacy_irq_number() == spurious_slave)
        IO::out8<master_command>(end_of_interrupt_code);

    return true;
}

void PIC::ensure_disabled()
{
    // just call the constructor, which already gets us to the state we're looking for
    new PIC();
}

void PIC::end_of_interrupt(u8 request_number)
{
    if (request_number >= 8)
        IO::out8<slave_command>(end_of_interrupt_code);

    IO::out8<master_command>(end_of_interrupt_code);
}

void PIC::clear_all()
{
    static constexpr u8 all_off_mask = 0xFF;

    set_raw_irq_mask(all_off_mask & ~SET_BIT(slave_irq_index), true);
    set_raw_irq_mask(all_off_mask, false);
}

void PIC::enable_irq_for(const IRQHandler& handler)
{
    ASSERT(handler.irq_handler_type() == IRQHandler::Type::LEGACY);
    auto number = handler.legacy_irq_number();

    u8 current_mask;

    if (number < 8) {
        current_mask = IO::in8<master_data>();
        IO::out8<master_data>(current_mask & ~(SET_BIT(number)));
    } else {
        current_mask = IO::in8<slave_data>();
        IO::out8<slave_data>(current_mask & ~(SET_BIT(number - 8)));
    }
}

void PIC::disable_irq_for(const IRQHandler& handler)
{
    ASSERT(handler.irq_handler_type() == IRQHandler::Type::LEGACY);
    auto number = handler.legacy_irq_number();

    u8 current_mask;

    if (number < 8) {
        current_mask = IO::in8<master_data>();
        IO::out8<master_data>(current_mask | SET_BIT(number));
    } else {
        current_mask = IO::in8<slave_data>();
        IO::out8<slave_data>(current_mask | SET_BIT(number - 8));
    }
}

void PIC::set_raw_irq_mask(u8 mask, bool master)
{
    if (master)
        IO::out8<master_data>(mask);
    else
        IO::out8<slave_data>(mask);
}

bool PIC::is_irq_being_serviced(u8 request_number)
{
    static constexpr u8 read_isr = 0x0b;

    IO::out8<master_command>(read_isr);
    IO::out8<slave_command>(read_isr);

    u16 isr_mask = (IO::in8<slave_command>() << 8) | IO::in8<master_command>();

    info() << "PIC: ISR mask=" << isr_mask << " request number=" << request_number;

    return IS_BIT_SET(isr_mask, request_number);
}

void PIC::remap(u8 offset)
{
    u8 master_mask = IO::in8<master_data>();
    u8 slave_mask = IO::in8<slave_data>();

    static constexpr u8 icw1_icw4 = 0x01;
    static constexpr u8 icw1_init = 0x10;
    static constexpr u8 icw4_8086 = 0x01;

    static constexpr u8 irqs_per_controller = 8;

    static constexpr u8 slave_irq = 0b00000100;
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
