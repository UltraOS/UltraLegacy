#include "Core/CPU.h"

#include "IDT.h"
#include "LAPIC.h"

#include "IPICommunicator.h"

// Defined in Arcitecture/X/Interrupts.asm
extern "C" void ipi_handler();

namespace kernel {

void IPICommunicator::install()
{
    IDT::the().register_interrupt_handler(vector_number, &ipi_handler);
}

void IPICommunicator::send_ipi(u8 dest)
{
    ASSERT(InterruptController::the().supports_smp());

    LAPIC::send_ipi<LAPIC::DestinationType::SPECIFIC>(dest);
}

void IPICommunicator::hang_all_cores()
{
    if (InterruptController::the().supports_smp())
        LAPIC::send_ipi<LAPIC::DestinationType::ALL_EXCLUDING_SELF>();
}

void IPICommunicator::on_ipi(RegisterState*)
{
    // TODO: At the moment IPIs are only used as a panic mechanism.
    //       Add a separate handler for panic later or add some kind of a
    //       message passing system.
    hang();

    LAPIC::end_of_interrupt();
}
}
