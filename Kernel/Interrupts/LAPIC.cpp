#include "Core/CPU.h"

#include "Memory/MemoryManager.h"

#include "Multitasking/Process.h"
#include "Multitasking/Scheduler.h"

#include "IPICommunicator.h"
#include "IRQManager.h"
#include "Timer.h"

#include "LAPIC.h"

namespace kernel {

Address LAPIC::s_base;

void LAPIC::set_base_address(Address physical_base)
{
#ifdef ULTRA_32
    auto region = MemoryManager::the().allocate_kernel_non_owning("LAPIC"_sv, Range(physical_base, Page::size));
    region->make_eternal();
    s_base = region->virtual_range().begin();
#elif defined(ULTRA_64)
    s_base = MemoryManager::physical_to_virtual(physical_base);
#endif
}

void LAPIC::initialize_for_this_processor()
{
    auto current_value = read_register(Register::SPURIOUS_INTERRUPT_VECTOR);

    static constexpr u32 enable_bit = 0b100000000;

    write_register(Register::SPURIOUS_INTERRUPT_VECTOR, current_value | enable_bit | spurious_irq_index);

    auto& smp_data = InterruptController::smp_data();

    auto my_info = linear_search(smp_data.lapics.begin(), smp_data.lapics.end(), my_id(),
        [](const LAPICInfo& info, u32 id) {
            return info.id == id;
        });

    ASSERT(my_info != smp_data.lapics.end());

    if (my_info->nmi_connection) {
        static constexpr u32 nmi_bit = 0b10000000000;

        auto& nmi = my_info->nmi_connection.value();

        // Both AMD and intel maunals say that trigger mode is hardcoded as edge for NMIs
        ASSERT(nmi.trigger_mode == decltype(nmi.trigger_mode)::EDGE);

        if (nmi.lint == 0) {
            write_register(Register::LVT_LOCAL_INTERRUPT_0, nmi_bit);
        } else if (nmi.lint == 1) {
            write_register(Register::LVT_LOCAL_INTERRUPT_1, nmi_bit);
        } else {
            FAILED_ASSERTION("LINT can either be 0 or 1");
        }
    }
}

void LAPIC::Timer::calibrate_for_this_processor()
{
    auto& primary_timer = primary();

    LOCK_GUARD(primary_timer.lock());

    static constexpr u32 divider_16 = 0b11;
    // TODO: replace with numeric_limits<u32>::max()
    static constexpr u32 initial_counter = 0xFFFFFFFF;

    static constexpr u32 sleep_delay = Time::milliseconds_in_second / default_ticks_per_second;

    write_register(Register::DIVIDE_CONFIGURATION, divider_16);
    write_register(Register::INITIAL_COUNT, initial_counter);

    primary_timer.mili_delay(sleep_delay);

    auto total_ticks = initial_counter - read_register(Register::CURRENT_COUNT);

    write_register(Register::LVT_TIMER, irq_number | periodic_mode | masked_bit);
    write_register(Register::INITIAL_COUNT, total_ticks);
}

void LAPIC::Timer::enable_irq()
{
    auto current_value = read_register(Register::LVT_TIMER);
    write_register(Register::LVT_TIMER, current_value & ~masked_bit);
}

void LAPIC::Timer::disable_irq()
{
    auto current_value = read_register(Register::LVT_TIMER);
    write_register(Register::LVT_TIMER, current_value | masked_bit);
}

void LAPIC::write_register(Register reg, u32 value)
{
    *Address(s_base + static_cast<size_t>(reg)).as_pointer<volatile u32>() = value;
}

u32 LAPIC::read_register(Register reg)
{
    return *Address(s_base + static_cast<size_t>(reg)).as_pointer<volatile u32>();
}

void LAPIC::end_of_interrupt()
{
    write_register(Register::END_OF_INTERRUPT, 0);
}

u32 LAPIC::my_id()
{
    return (read_register(Register::ID) >> 24u) & 0xFFu;
}

void LAPIC::ICR::post()
{
    u32 icr_as_u32[2];
    copy_memory(this, icr_as_u32, sizeof(*this));

    write_register(Register::INTERRUPT_COMMAND_REGISTER_HIGHER, icr_as_u32[1]);
    write_register(Register::INTERRUPT_COMMAND_REGISTER_LOWER, icr_as_u32[0]);
}

void LAPIC::send_ipi(DestinationType destination_type, u32 destination)
{
    if (destination_type == DestinationType::SPECIFIC && destination == invalid_destination)
        runtime::panic("LAPIC: invalid destination with destination type == DEFAULT");

    ICR icr {};

    icr.vector_number = IPICommunicator::vector_number;
    icr.delivery_mode = DeliveryMode::NORMAL;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level = Level::ASSERT;
    icr.trigger_mode = TriggerMode::EDGE;
    icr.destination_type = destination_type;
    icr.destination_id = destination;

    icr.post();
}

void LAPIC::send_init_to(u8 id)
{
    ICR icr {};

    icr.delivery_mode = DeliveryMode::INIT;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level = Level::ASSERT;
    icr.trigger_mode = TriggerMode::EDGE;
    icr.destination_type = DestinationType::SPECIFIC;
    icr.destination_id = id;

    icr.post();
}

void LAPIC::send_startup_to(u8 id)
{
    static constexpr u8 entrypoint_page_number = 1;

    ICR icr {};

    icr.entrypoint_page = entrypoint_page_number;
    icr.delivery_mode = DeliveryMode::SIPI;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level = Level::ASSERT;
    icr.trigger_mode = TriggerMode::EDGE;
    icr.destination_type = DestinationType::SPECIFIC;
    icr.destination_id = id;

    icr.post();
}

// defined in Architecture/X/APTrampoline.asm
extern "C" void application_processor_entrypoint();

void LAPIC::start_processor(u8 id)
{
    log() << "LAPIC: Starting application processor " << id << "...";

    static constexpr Address entrypoint_base = 0x1000;
    static constexpr Address entrypoint_base_linear = MemoryManager::physical_to_virtual(entrypoint_base);

    static bool entrypoint_relocated = false;

    static constexpr ptr_t address_of_alive = 0x500;
    static constexpr ptr_t address_of_acknowldeged = 0x510;
    static constexpr ptr_t address_of_stack = 0x520;

#ifdef ULTRA_64
    static constexpr ptr_t address_of_pml4 = 0x530;
    static constexpr Address pml4_linear = MemoryManager::physical_to_virtual(address_of_pml4);

    // Identity map the first pdpt for the AP
    AddressSpace::of_kernel().entry_at(0) = AddressSpace::of_kernel().entry_at(256);
#endif

    volatile auto* is_ap_alive = MemoryManager::physical_to_virtual(address_of_alive).as_pointer<bool>();
    volatile auto* ap_acknowledegd = MemoryManager::physical_to_virtual(address_of_acknowldeged).as_pointer<bool>();

    *is_ap_alive = false;
    *ap_acknowledegd = false;

    String stack_name = "core ";
    stack_name << id << " idle task stack";

    auto ap_stack = MemoryManager::the().allocate_kernel_stack(stack_name.to_view());

    *MemoryManager::physical_to_virtual(address_of_stack).as_pointer<ptr_t>() = ap_stack->virtual_range().end();

    if (!entrypoint_relocated) {
        copy_memory(reinterpret_cast<void*>(&application_processor_entrypoint),
            entrypoint_base_linear.as_pointer<void>(),
            Page::size);

#ifdef ULTRA_64
        *pml4_linear.as_pointer<ptr_t>() = AddressSpace::of_kernel().physical_address();
#endif
        entrypoint_relocated = true;
    }

    auto& timer = Timer::primary();
    LOCK_GUARD(timer.lock());

    send_init_to(id);

    timer.mili_delay(10);

    send_startup_to(id);

    timer.mili_delay(1);

    if (*is_ap_alive) {
        log() << "LAPIC: Application processor " << id << " started successfully";

        // allow the ap to boot further
        *ap_acknowledegd = true;
        return;
    }

    send_startup_to(id);

    // if we're here we're probably dealing with an old CPU that needs two SIPIs

    // wait for 1 second
    for (size_t i = 0; i < 20; ++i)
        timer.mili_delay(50);

    if (*is_ap_alive) {
        log() << "LAPIC: Application processor " << id << " started successfully after second SIPI";

        // allow the ap to boot further
        *ap_acknowledegd = true;

    } else {
        String error_string;
        error_string << "LAPIC: Application processor " << id << " failed to start after 2 SIPIs";
        runtime::panic(error_string.data());
    }
}
}
