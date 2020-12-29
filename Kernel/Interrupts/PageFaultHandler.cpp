#include "PageFaultHandler.h"
#include "Memory/MemoryManager.h"

namespace kernel {

PageFaultHandler PageFaultHandler::s_instance;

PageFaultHandler::PageFaultHandler()
    : ExceptionHandler(exception_number)
{
}

void PageFaultHandler::handle(const RegisterState& state)
{
    ++m_occurrences;

    static constexpr u32 type_mask = 0b011;
    static constexpr u32 user_mask = 0b100;

    Address address_of_fault;
    asm("mov %%cr2, %0"
        : "=a"(address_of_fault));

    PageFault pf(
        address_of_fault,
        state.instruction_pointer(),
        (state.error_code & user_mask) ? IsSupervisor::NO : IsSupervisor::YES,
        static_cast<PageFault::Type>(state.error_code & type_mask));

    MemoryManager::handle_page_fault(state, pf);
}
}
