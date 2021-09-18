#include "GDT.h"

#include "Common/Utilities.h"

#include "Interrupts/IDT.h"
#include "Interrupts/InterruptController.h"
#include "Interrupts/LAPIC.h"

#include "Memory/MemoryManager.h"
#include "Memory/PAT.h"

#include "Multitasking/Process.h"
#include "Multitasking/TSS.h"

#include "CPU.h"
#include "FPU.h"

namespace kernel {

Atomic<size_t> CPU::s_alive_counter;
List<CPU::LocalData> CPU::s_processors;
CPU::LocalData* CPU::s_id_to_processor[CPU::max_lapic_id + 1] {};

CPU::LocalData::LocalData(u32 id)
    : m_id(id)
{
    m_is_online = new Atomic<bool>(false);
    m_request_lock = new InterruptSafeSpinLock;
}

IPICommunicator::Request* CPU::LocalData::pop_request()
{
    bool interrupt_state = false;
    m_request_lock->lock(interrupt_state, __FILE__, __LINE__);

    IPICommunicator::Request* request = nullptr;

    if (!m_requests.empty())
        request = m_requests.dequeue();

    m_request_lock->unlock(interrupt_state);
    return request;
}

void CPU::LocalData::push_request(IPICommunicator::Request& request)
{
    bool interrupt_state = false;
    m_request_lock->lock(interrupt_state, __FILE__, __LINE__);

    if (m_requests.full())
        runtime::panic("IPI request queue full");

    m_requests.enqueue(&request);

    m_request_lock->unlock(interrupt_state);
}

Thread& CPU::LocalData::idle_task()
{
    return **m_idle_process->threads().begin();
}

CPU::ID::ID(u32 function)
{
    asm volatile("cpuid"
                 : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                 : "a"(function), "c"(0));
}

CPU::MSR CPU::MSR::read(u32 index)
{
    MSR out;

    asm volatile("rdmsr"
                 : "=d"(out.upper), "=a"(out.lower)
                 : "c"(index));

    return out;
}

void CPU::MSR::write(u32 index)
{
    asm volatile("wrmsr" ::"d"(upper), "a"(lower), "c"(index));
}

CPU::XCR CPU::XCR::read(u32 index)
{
    XCR out;

    asm volatile("xgetbv"
                 : "=d"(out.upper), "=a"(out.lower)
                 : "c"(index));

    return out;
}

void CPU::XCR::write(u32 index)
{
    asm volatile("xsetbv" ::"d"(upper), "a"(lower), "c"(index));
}

void CPU::write_cr4(size_t value)
{
    asm volatile("mov %0, %%cr4" ::"r"(value));
}

size_t CPU::read_cr4()
{
    size_t value;
    asm volatile("mov %%cr4, %0"
                 : "=r"(value));

    return value;
}

void CPU::initialize()
{
    u32 bsp_id = 0;

    if (supports_smp())
        bsp_id = LAPIC::my_id();

    auto cpu = s_id_to_processor[bsp_id] = new LocalData(bsp_id);
    s_processors.insert_back(*cpu);
    cpu->bring_online();

    s_alive_counter.fetch_add(1, MemoryOrder::ACQ_REL);
}

CPU::FLAGS CPU::flags()
{
    FLAGS flags;
    asm volatile("pushf\n"
                 "pop %0\n"
                 : "=a"(flags)::"memory");

    return flags;
}

bool CPU::supports_smp()
{
    return !InterruptController::is_legacy_mode();
}

void CPU::start_all_processors()
{
    if (InterruptController::is_legacy_mode())
        return;

    // We have to do this here because otherwise we have a race condition
    // with each cpu emplacing their local data upon entering ap_entrypoint.
    // Locking is also not an option because CPU::current would have to use this lock
    // as well, which would lead to recursively calling current() everywhere.
    auto bsp_id = InterruptController::smp_data().bsp_lapic_id;
    auto& lapics = InterruptController::smp_data().lapics;
    for (const auto& lapic : lapics) {
        if (lapic.id == bsp_id)
            continue;

        auto cpu = s_id_to_processor[lapic.id] = new LocalData(lapic.id);
        s_processors.insert_back(*cpu);
    }

    for (const auto& lapic : lapics) {
        if (lapic.id == bsp_id)
            continue;

        LAPIC::start_processor(lapic.id);

        auto& this_processor = CPU::at_id(lapic.id);

        while (!this_processor.is_online()) {
            pause();
        }
    }

    // now LAPIC is the primary timer
    Timer::get_specific(LAPIC::Timer::type).make_primary();
    Timer::primary().enable(); // enable for the BSP
}

CPU::LocalData& CPU::at_id(u32 id)
{
    if (id > max_lapic_id || !s_id_to_processor[id]) {
        String error_string;
        error_string << "CPU: Couldn't find the local data for cpu " << id;
        runtime::panic(error_string.data());
    }

    return *s_id_to_processor[id];
}

CPU::LocalData& CPU::current()
{
    if (InterruptController::is_initialized() && supports_smp())
        return at_id(LAPIC::my_id());
    else
        return at_id(0);
}

[[nodiscard]] Process& CPU::LocalData::current_process() const
{
    ASSERT(m_current_thread != nullptr);
    return m_current_thread->owner();
}

u32 CPU::current_id()
{
    if (is_initialized() && supports_smp())
        return LAPIC::my_id();

    return 0;
}

void CPU::LocalData::bring_online()
{
    ASSERT(m_is_online->load(MemoryOrder::ACQUIRE) == false);

    if (supports_smp())
        ASSERT(LAPIC::my_id() == m_id);

    m_is_online->store(true, MemoryOrder::RELEASE);
}

void CPU::ap_entrypoint()
{
    GDT::the().install();
    IDT::the().install();
    PAT::the().synchronize();
    FPU::initialize_for_this_cpu();

    LAPIC::initialize_for_this_processor();
    auto& timer = Timer::get_specific(LAPIC::Timer::type);
    timer.calibrate_for_this_processor();
    timer.enable();

    Process::create_idle_for_this_processor();

    auto& current_cpu = CPU::current();
    current_cpu.set_tss(*new TSS);

    current_cpu.bring_online();
    s_alive_counter.fetch_add(1, MemoryOrder::ACQ_REL);

    Interrupts::enable();

    for (;;)
        hlt();
}
}
