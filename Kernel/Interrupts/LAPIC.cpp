#include "Memory/MemoryManager.h"

#include "Multitasking/Process.h"

#include "IPICommunicator.h"
#include "Timer.h"

#include "LAPIC.h"

namespace kernel {

Address LAPIC::s_base;

void LAPIC::set_base_address(Address physical_base)
{
#ifdef ULTRA_32
    s_base = AddressSpace::of_kernel().allocator().allocate_range(4096).begin();

    AddressSpace::of_kernel().map_page(s_base, physical_base);
#elif defined(ULTRA_64)
    s_base = MemoryManager::physical_to_virtual(physical_base);
#endif
}

void LAPIC::initialize_for_this_processor()
{
    auto current_value = read_register(Register::SPURIOUS_INTERRUPT_VECTOR);

    static constexpr u32 enable_bit = 0b100000000;

    write_register(Register::SPURIOUS_INTERRUPT_VECTOR, current_value | enable_bit | spurious_irq_index);
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
    return (read_register(Register::ID) >> 24) & 0xFF;
}

void LAPIC::send_ipi(DestinationType destination_type, u32 destination)
{
    if (destination_type == DestinationType::SPECIFIC && destination == invalid_destination) {
        error() << "LAPIC: invalid destination with destination type == DEFAULT";
        hang();
    }

    ICR icr {};

    icr.vector_number    = IPICommunicator::vector_number;
    icr.delivery_mode    = DeliveryMode::NORMAL;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level            = Level::ASSERT;
    icr.trigger_mode     = TriggerMode::EDGE;
    icr.destination_type = destination_type;
    icr.destination_id   = destination;

    volatile auto* icr_pointer = Address(&icr).as_pointer<volatile u32>();

    write_register(Register::INTERRUPT_COMMAND_REGISTER_HIGHER, *(icr_pointer + 1));
    write_register(Register::INTERRUPT_COMMAND_REGISTER_LOWER, *icr_pointer);
}

void LAPIC::send_init_to(u8 id)
{
    ICR icr {};

    icr.delivery_mode    = DeliveryMode::INIT;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level            = Level::ASSERT;
    icr.trigger_mode     = TriggerMode::EDGE;
    icr.destination_type = DestinationType::SPECIFIC;
    icr.destination_id   = id;

    volatile auto* icr_pointer = Address(&icr).as_pointer<volatile u32>();

    write_register(Register::INTERRUPT_COMMAND_REGISTER_HIGHER, *(icr_pointer + 1));
    write_register(Register::INTERRUPT_COMMAND_REGISTER_LOWER, *icr_pointer);
}

void LAPIC::send_startup_to(u8 id)
{
    static constexpr u8 entrypoint_page_number = 1;

    ICR icr {};

    icr.entrypoint_page  = entrypoint_page_number;
    icr.delivery_mode    = DeliveryMode::SIPI;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level            = Level::ASSERT;
    icr.trigger_mode     = TriggerMode::EDGE;
    icr.destination_type = DestinationType::SPECIFIC;
    icr.destination_id   = id;

    volatile auto* icr_pointer = Address(&icr).as_pointer<volatile u32>();

    write_register(Register::INTERRUPT_COMMAND_REGISTER_HIGHER, *(icr_pointer + 1));
    write_register(Register::INTERRUPT_COMMAND_REGISTER_LOWER, *icr_pointer);
}

// defined in Architecture/X/APTrampoline.asm
extern "C" void application_processor_entrypoint();

void LAPIC::start_processor(u8 id)
{
    log() << "LAPIC: Starting application processor " << id << "...";

    static constexpr Address entrypoint_base        = 0x1000;
    static constexpr Address entrypoint_base_linear = MemoryManager::physical_to_virtual(entrypoint_base);

    static bool entrypoint_relocated = false;

    static constexpr ptr_t address_of_alive        = 0x500;
    static constexpr ptr_t address_of_acknowldeged = 0x510;
    static constexpr ptr_t address_of_stack        = 0x520;

#ifdef ULTRA_64
    static constexpr ptr_t   address_of_pml4 = 0x530;
    static constexpr Address pml4_linear     = MemoryManager::physical_to_virtual(address_of_pml4);

    // Identity map the first pdpt for the AP
    AddressSpace::of_kernel().entry_at(0) = AddressSpace::of_kernel().entry_at(256);
#endif

    auto* is_ap_alive     = MemoryManager::physical_to_virtual(address_of_alive).as_pointer<bool>();
    auto* ap_acknowledegd = MemoryManager::physical_to_virtual(address_of_acknowldeged).as_pointer<bool>();

    // TODO: add a literal allocate_stack() function
    // allocate the initial AP stack
    auto ap_stack = AddressSpace::of_kernel().allocator().allocate_range(Process::default_kerenl_stack_size);
    *MemoryManager::physical_to_virtual(address_of_stack).as_pointer<ptr_t>() = ap_stack.end();

    // Force a page fault to actually back this with a physical page as the AP doesn't have the IDT set up yet
    *Address(ap_stack.end() - 1).as_pointer<u8>() = 0xFF;

    if (!entrypoint_relocated) {
        copy_memory(reinterpret_cast<void*>(&application_processor_entrypoint),
                    entrypoint_base_linear.as_pointer<void>(),
                    Page::size);

#ifdef ULTRA_64
        *pml4_linear.as_pointer<ptr_t>() = AddressSpace::of_kernel().physical_address();
#endif
        entrypoint_relocated = true;
    }

    send_init_to(id);

    Timer::the().mili_delay(10);

    send_startup_to(id);

    Timer::the().mili_delay(1);

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
        Timer::the().mili_delay(50);

    if (*is_ap_alive) {
        log() << "LAPIC: Application processor " << id << " started successfully after second SIPI";

        // allow the ap to boot further
        *ap_acknowledegd = true;

    } else
        error() << "LAPIC: Application processor " << id << " failed to start after 2 SIPIs";
}
}
