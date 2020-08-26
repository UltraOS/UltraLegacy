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
    LAPIC::send_ipi<LAPIC::DestinationType::SPECIFIC>(dest);
}

void IPICommunicator::on_ipi(RegisterState*)
{
    log() << "Got an ipi on cpu " << CPU::current().id();
    LAPIC::end_of_interrupt();
}
}
