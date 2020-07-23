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

u32 LAPIC::my_id()
{
    return (read_register(Register::ID) >> 24) & 0xFF;
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

extern "C" bool alive;
extern "C" bool allowed_to_boot;

void LAPIC::start_processor(u8 id)
{
    log() << "LAPIC: Starting application processor " << id << "...";

    static bool              entrypoint_relocated   = false;
    static constexpr Address entrypoint_base        = 0x1000;
    static constexpr Address entrypoint_base_linear = MemoryManager::physical_address_as_kernel(entrypoint_base);

    auto address_of_var = [](auto addr) {
        size_t offset = Address(addr).as_pointer<volatile u8>()
                        - reinterpret_cast<volatile u8*>(&application_processor_entrypoint);

        Address var_address = entrypoint_base.as_pointer<volatile u8>() + offset;

        return MemoryManager::physical_address_as_kernel(var_address)
            .as_pointer<volatile remove_pointer_t<decltype(addr)>>();
    };

    auto* is_ap_alive        = address_of_var(&alive);
    auto* is_allowed_to_boot = address_of_var(&allowed_to_boot);

    if (!entrypoint_relocated) {
        copy_memory(reinterpret_cast<void*>(&application_processor_entrypoint),
                    entrypoint_base_linear.as_pointer<void>(),
                    Page::size);
        entrypoint_relocated = true;
    }

    send_init_to(id);

    Timer::the().mili_delay(10);

    send_startup_to(id);

    Timer::the().mili_delay(1);

    if (*is_ap_alive) {
        log() << "LAPIC: Application processor " << id << " started successfully";

        // allow the ap to boot further
        *is_allowed_to_boot = true;

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
        *is_allowed_to_boot = true;

    } else
        error() << "LAPIC: Application processor " << id << " failed to start after 2 SIPIs";
}

extern "C" void ap_entrypoint()
{

    static size_t row = 7;

    auto offset = vga_log("Hello from core ", row, 0, 0xF);

    char buf[4];
    to_string(LAPIC::my_id(), buf, 4);

    vga_log(buf, row++, offset, 0xf);

    hang();
}
}
