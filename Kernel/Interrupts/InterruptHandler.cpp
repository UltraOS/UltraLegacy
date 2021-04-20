#include "InterruptHandler.h"
#include "InterruptManager.h"

namespace kernel {

RangedInterruptHandler::RangedInterruptHandler(VectorRange range, DynamicArray<u16> user_callable_vectors)
    : m_range(range)
    , m_user_vectors(move(user_callable_vectors))
{
    IVectorAllocator::the().allocate_range(range);
    InterruptManager::register_handler(*this);
}

RangedInterruptHandler::~RangedInterruptHandler()
{
    IVectorAllocator::the().free_range(m_range);
}

MonoInterruptHandler::MonoInterruptHandler(u16 vector, bool user_callable)
    : m_user_callable(user_callable)
{
    if (vector == any_vector)
        m_vector = IVectorAllocator::the().allocate_vector();
    else {
        m_vector = vector;
        IVectorAllocator::the().allocate_vector(vector);
    }

    InterruptManager::register_handler(*this);
}

MonoInterruptHandler::~MonoInterruptHandler()
{
    IVectorAllocator::the().free_vector(interrupt_vector());
}

u16 DynamicInterruptHandler::allocate_one(u16 vector, bool user_callable)
{
    if (vector == any_vector)
        vector = IVectorAllocator::the().allocate_vector();
    else
        IVectorAllocator::the().allocate_vector(vector);

    InterruptManager::register_dynamic_handler(*this, vector, user_callable);

    return vector;
}

void DynamicInterruptHandler::free_one(u16 vector)
{
    InterruptManager::unregister_dynamic_handler(*this, vector);
}

}
