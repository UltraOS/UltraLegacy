#include "APIC.h"

namespace kernel {

APIC::APIC(Address physical_base) : m_base(PageDirectory::of_kernel().allocator().allocate_range(4096).begin())
{
    PageDirectory::of_kernel().map_page(m_base, physical_base);

    auto current_value = read_register(Register::SPURIOUS_INTERRUPT_VECTOR);

    static constexpr u32 enable_bit = 0b100000000;
    static constexpr u32 spurious_irq_index = 0xFF;

    write_register(Register::SPURIOUS_INTERRUPT_VECTOR, current_value | enable_bit | spurious_irq_index);
}

void APIC::write_register(Register reg, u32 value)
{
    *Address(m_base + static_cast<size_t>(reg)).as_pointer<u32>() = value;
}

u32 APIC::read_register(Register reg)
{
    return *Address(m_base + static_cast<size_t>(reg)).as_pointer<u32>();
}
}
