#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Common/Types.h"

#include "Core/Registers.h"

#include "ExceptionDispatcher.h"
#include "ExceptionHandler.h"
#include "PageFaultHandler.h"

#include "Memory/MemoryManager.h"

namespace kernel {

ExceptionDispatcher* ExceptionDispatcher::s_instance;

void ExceptionDispatcher::initialize()
{
    ASSERT(s_instance == nullptr);

    s_instance = new ExceptionDispatcher();

    // Custom handlers go here
    new PageFaultHandler;
}

ExceptionDispatcher::ExceptionDispatcher()
    : RangedInterruptHandler({ 0, exception_count }, { 1, 3, 4 })
{
}

void ExceptionDispatcher::handle_interrupt(RegisterState& registers)
{
    auto exception_number = registers.interrupt_number;
    if (exception_number > 20)
        exception_number = 21; // security exception is 21st in the array

    if (m_handlers[exception_number] == nullptr) {
        String error_string;

        error_string << "An exception has occured: " << s_exception_messages[exception_number] << " ("
                     << exception_number << ")";

        runtime::panic(error_string.data(), &registers);
    }

    m_handlers[exception_number]->handle(registers);
}

void ExceptionDispatcher::register_handler(ExceptionHandler& handler)
{
    m_handlers[handler.exception_number()] = &handler;
}

void ExceptionDispatcher::unregister_handler(ExceptionHandler& handler)
{
    m_handlers[handler.exception_number()] = nullptr;
}
}
