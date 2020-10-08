#include "Core/Registers.h"

#include "Memory/MemoryManager.h"

#include "ExceptionHandler.h"
#include "PageFault.h"

namespace kernel {

class PageFaultHandler : public ExceptionHandler {
public:
    static constexpr auto exception_number = 0xE;

    PageFaultHandler() : ExceptionHandler(exception_number) { }

    void handle(const RegisterState& state) override
    {
        ++m_occurrences;

        static constexpr u32 type_mask = 0b011;
        static constexpr u32 user_mask = 0b100;

        Address address_of_fault;
        asm("mov %%cr2, %0" : "=a"(address_of_fault));

        PageFault pf(address_of_fault,
#ifdef ULTRA_32
                     state.eip,
#elif defined(ULTRA_64)
                     state.rip,
#endif
                     state.error_code & user_mask,
                     static_cast<PageFault::Type>(state.error_code & type_mask));

        MemoryManager::handle_page_fault(state, pf);

        log() << "PageFaultHandler: page fault resolved, continuing...";
    }

    size_t occurances() const { return m_occurrences; }

private:
    size_t                  m_occurrences { 0 };
    static PageFaultHandler s_instance;
};

PageFaultHandler PageFaultHandler::s_instance;
}
