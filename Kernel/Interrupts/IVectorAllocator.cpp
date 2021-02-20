#include "IVectorAllocator.h"
#include "IDT.h"
#include "Utilities.h"

namespace kernel {

IVectorAllocator* IVectorAllocator::s_instance;

void IVectorAllocator::initialize()
{
    ASSERT(s_instance == nullptr);
    s_instance = new IVectorAllocator;
}

IVectorAllocator::IVectorAllocator()
{
    m_allocation_map.set_size(IDT::entry_count);
}

void IVectorAllocator::allocate_vector(u16 vector)
{
    auto can_allocate = !m_allocation_map.bit_at(vector);
    ASSERT(can_allocate == true);

    m_allocation_map.set_bit(vector, true);
}

u16 IVectorAllocator::allocate_vector()
{
    static constexpr size_t allocation_base = legacy_irq_base + legacy_irq_count;

    auto bit = m_allocation_map.find_bit_starting_at_offset(false, allocation_base);
    ASSERT(bit.has_value());
    ASSERT(bit.value() >= allocation_base);

    m_allocation_map.set_bit(bit.value(), true);

    return bit.value();
}

void IVectorAllocator::allocate_range(VectorRange range)
{
    for (auto vector = range.begin(); vector < range.end(); ++vector)
        allocate_vector(vector);
}

void IVectorAllocator::free_vector(u16 vector)
{
    auto bit = m_allocation_map.bit_at(vector);
    ASSERT(bit == true);

    m_allocation_map.set_bit(vector, false);
}

void IVectorAllocator::free_range(VectorRange range)
{
    for (auto vector = range.begin(); vector < range.end(); ++vector)
        free_vector(vector);
}

}
