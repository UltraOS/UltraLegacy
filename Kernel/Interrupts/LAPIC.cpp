#include "LAPIC.h"
#include "Memory/MemoryManager.h"
#include "Timer.h"

namespace kernel {

Address LAPIC::s_base;

void LAPIC::set_base_address(Address physical_base)
{
    s_base = PageDirectory::of_kernel().allocator().allocate_range(4096).begin();

    PageDirectory::of_kernel().map_page(s_base, physical_base);
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

extern "C" void application_processor_entrypoint();

void LAPIC::send_init_to(u8 id)
{
    ICR icr {};

    icr.delivery_mode    = DeliveryMode::INIT;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level            = Level::ASSERT;
    icr.trigger_mode     = TriggerMode::EDGE;
    icr.destination_type = DestinationType::DEFAULT;
    icr.destination_id   = id;

    volatile auto* icr_pointer = Address(&icr).as_pointer<volatile u32>();

    write_register(Register::INTERRUPT_COMMAND_REGISTER_HIGHER, *(icr_pointer + 1));
    write_register(Register::INTERRUPT_COMMAND_REGISTER_LOWER, *icr_pointer);
}

void LAPIC::send_startup_to(u8 id)
{
    ICR icr {};

    icr.entrypoint_page  = 1;
    icr.delivery_mode    = DeliveryMode::SIPI;
    icr.destination_mode = DestinationMode::PHYSICAL;
    icr.level            = Level::ASSERT;
    icr.trigger_mode     = TriggerMode::EDGE;
    icr.destination_type = DestinationType::DEFAULT;
    icr.destination_id   = id;

    volatile auto* icr_pointer = Address(&icr).as_pointer<volatile u32>();

    write_register(Register::INTERRUPT_COMMAND_REGISTER_HIGHER, *(icr_pointer + 1));
    write_register(Register::INTERRUPT_COMMAND_REGISTER_LOWER, *icr_pointer);
}

void LAPIC::start_processor(u8 id)
{
    log() << "LAPIC: Starting application processor " << id << "...";

    static bool              entrypoint_relocated   = false;
    static constexpr Address entrypoint_base        = 0x1000;
    static constexpr Address entrypoint_base_linear = MemoryManager::physical_address_as_kernel(entrypoint_base);

    if (!entrypoint_relocated) {
        copy_memory(reinterpret_cast<void*>(&application_processor_entrypoint),
                    entrypoint_base_linear.as_pointer<void>(),
                    Page::size);
        entrypoint_relocated = true;
    }

    volatile auto* magic_memory_address
        = Address(MemoryManager::physical_address_as_kernel(0x2000)).as_pointer<volatile u16>();
    *magic_memory_address = 0xBEEF;

    send_init_to(id);

    Timer::the().mili_delay(10);

    send_startup_to(id);

    Timer::the().mili_delay(1);

    if (*magic_memory_address == 0xDEAD) {
        log() << "LAPIC: Application processor " << id << " started successfully";

        char str[2];
        to_string(id, str, 2);
        auto offset = vga_log("Processor ", 8 + id, 0, 0x3);
        offset      = vga_log(str, 8 + id, offset, 0x3);
        vga_log(" started successfully!", 8 + id, offset, 0x3);

        return;
    }

    send_startup_to(id);

    // if we're here we're probably dealing with an old CPU that needs two SIPIs

    // wait for 1 second
    for (size_t i = 0; i < 20; ++i)
        Timer::the().mili_delay(50);

    if (*magic_memory_address == 0xDEAD) {
        char str[2];
        to_string(id, str, 2);
        auto offset = vga_log("Processor ", 8 + id, 0, 0x3);
        offset      = vga_log(str, 8 + id, offset, 0x3);
        vga_log(" started successfully after two SIPIs!", 8 + id, offset, 0x3);
        log() << "LAPIC: Application processor " << id << " started successfully after second SIPI";
    } else
        error() << "LAPIC: Application processor " << id << " failed to start after 2 SIPIs";
}
}
