#include "LAPIC.h"

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
    static constexpr u32 spurious_irq_index = 0xFF;

    write_register(Register::SPURIOUS_INTERRUPT_VECTOR, current_value | enable_bit | spurious_irq_index);
}

void LAPIC::write_register(Register reg, u32 value)
{
    *Address(s_base + static_cast<size_t>(reg)).as_pointer<u32>() = value;
}

u32 LAPIC::read_register(Register reg)
{
    return *Address(s_base + static_cast<size_t>(reg)).as_pointer<u32>();
}
}
