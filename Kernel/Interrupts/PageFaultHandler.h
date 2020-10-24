#include "Core/Registers.h"
#include "ExceptionHandler.h"
#include "PageFault.h"

namespace kernel {

class PageFaultHandler : public ExceptionHandler {
public:
    static constexpr auto exception_number = 0xE;

    PageFaultHandler() : ExceptionHandler(exception_number) { }

    void handle(const RegisterState& state) override;

    size_t occurances() const { return m_occurrences; }

private:
    size_t                  m_occurrences { 0 };
    static PageFaultHandler s_instance;
};

PageFaultHandler PageFaultHandler::s_instance;
}
