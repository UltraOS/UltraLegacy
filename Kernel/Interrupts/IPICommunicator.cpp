#include "Core/CPU.h"

#include "IDT.h"
#include "LAPIC.h"

#include "IPICommunicator.h"

namespace kernel {

IPICommunicator* IPICommunicator::s_instance;

void IPICommunicator::initialize()
{
    ASSERT(s_instance == nullptr);
    s_instance = new IPICommunicator;
}

IPICommunicator::IPICommunicator()
    : MonoInterruptHandler(vector_number, false)
{
}

void IPICommunicator::send_ipi(u8 dest)
{
    ASSERT(!InterruptController::is_legacy_mode());

    LAPIC::send_ipi<LAPIC::DestinationType::SPECIFIC>(dest);
}

void IPICommunicator::hang_all_cores()
{
    if (CPU::alive_processor_count() == 1) // nothing to hang, we're the only cpu alive
        return;

    // FIXME: This is terrible because not all cpus might be online, also
    //        there can be cpus marked as unusable making this IPI call UB
    if (!InterruptController::is_legacy_mode())
        LAPIC::send_ipi<LAPIC::DestinationType::ALL_EXCLUDING_SELF>();
}

void IPICommunicator::handle_interrupt(RegisterState&)
{
    // TODO: At the moment IPIs are only used as a panic mechanism.
    //       Add a separate handler for panic later or add some kind of a
    //       message passing system.
    hang();

    LAPIC::end_of_interrupt();
}
}
