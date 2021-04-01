#include "Core/CPU.h"

#include "IDT.h"
#include "LAPIC.h"

#include "IPICommunicator.h"
#include "Multitasking/Scheduler.h"

namespace kernel {

IPICommunicator* IPICommunicator::s_instance;

void IPICommunicator::initialize()
{
    ASSERT(s_instance == nullptr);
    s_instance = new IPICommunicator;
}

void IPICommunicator::Request::wait_for_completion()
{
    u64 request_timeout_counter = 1'000'000'000;

    while (m_completion_countdown.load() != 0 && --request_timeout_counter) {
        // Process the queue while we wait for other cpus anyways
        the().process_pending();
        pause();
    }

    if (m_completion_countdown.load() == 0)
        return;

    // This was a hang request probably sent from runtime::panic, so we don't wanna panic again
    if (type() == Type::HANG) {
        error() << "IPICommunicator: A core has failed to complete an IPI task, countdown = "
                << m_completion_countdown.load();
    } else {
        String error_string;
        error_string << "IPICommunicator: request timeout, " << m_completion_countdown.load()
                     << " core(s) failed to complete request of type " << static_cast<int>(type());
        runtime::panic(error_string.c_string());
    }
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

void IPICommunicator::post_request(Request& request)
{
    // Called too early or one cpu system
    if (!InterruptController::is_initialized() || !CPU::is_initialized() || CPU::alive_processor_count() == 1)
        return;

    Interrupts::ScopedDisabler d;

    auto current_id = CPU::current_id();

    for (auto& cpu : CPU::processors()) {
        if (cpu.second().id() == current_id || !cpu.second().is_online())
            continue;

        request.increment_completion_countdown();

        cpu.second().push_request(request);
        send_ipi(cpu.second().id());
    }
}

void IPICommunicator::process_pending()
{
    auto& current_cpu = CPU::current();

    while (auto* request = current_cpu.pop_request()) {
        switch (request->type()) {
        case Request::Type::HANG:
            hang();
        case Request::Type::INVALIDATE_RANGE: {
            auto* invalidation_request = static_cast<RangeInvalidationRequest*>(request);
            AddressSpace::current().invalidate_range(invalidation_request->virtual_range());
            break;
        }
        default:
            ASSERT_NEVER_REACHED();
        }

        request->complete();
    }
}

void IPICommunicator::handle_interrupt(RegisterState&)
{
    process_pending();
    LAPIC::end_of_interrupt();
}
}
