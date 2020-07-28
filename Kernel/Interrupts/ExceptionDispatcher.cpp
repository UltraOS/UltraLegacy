#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Common/Types.h"

#include "Common.h"
#include "IDT.h"
#include "ExceptionDispatcher.h"
#include "ExceptionHandler.h"

#include "Memory/MemoryManager.h"

EXCEPTION_HANDLER_SYMBOL(0);
EXCEPTION_HANDLER_SYMBOL(1);
EXCEPTION_HANDLER_SYMBOL(2);
EXCEPTION_HANDLER_SYMBOL(3);
EXCEPTION_HANDLER_SYMBOL(4);
EXCEPTION_HANDLER_SYMBOL(5);
EXCEPTION_HANDLER_SYMBOL(6);
EXCEPTION_HANDLER_SYMBOL(7);
EXCEPTION_HANDLER_SYMBOL(8);
EXCEPTION_HANDLER_SYMBOL(9);
EXCEPTION_HANDLER_SYMBOL(10);
EXCEPTION_HANDLER_SYMBOL(11);
EXCEPTION_HANDLER_SYMBOL(12);
EXCEPTION_HANDLER_SYMBOL(13);
EXCEPTION_HANDLER_SYMBOL(14);
EXCEPTION_HANDLER_SYMBOL(16);
EXCEPTION_HANDLER_SYMBOL(17);
EXCEPTION_HANDLER_SYMBOL(18);
EXCEPTION_HANDLER_SYMBOL(19);
EXCEPTION_HANDLER_SYMBOL(20);
EXCEPTION_HANDLER_SYMBOL(30);

namespace kernel {

ExceptionHandler* ExceptionDispatcher::s_handlers[ExceptionDispatcher::exception_count];

void ExceptionDispatcher::exception_handler(RegisterState* registers)
{
    auto exception_number = registers->exception_number;
    if (exception_number > 20)
        exception_number = 21; // security exception is 21st in the array

    if (s_handlers[exception_number] == nullptr) {
        error() << "An exception has occured: " << s_exception_messages[exception_number]
                << " (" << exception_number << ")";
        hang();
    }

    s_handlers[exception_number]->handle(*registers);
}

void ExceptionDispatcher::register_handler(ExceptionHandler& handler)
{
    s_handlers[handler.exception_number()] = &handler;
}

void ExceptionDispatcher::unregister_handler(ExceptionHandler& handler)
{
    s_handlers[handler.exception_number()] = nullptr;
}

void ExceptionDispatcher::install()
{
    IDT::the()
        .register_interrupt_handler(0, exception0_handler)
        .register_user_interrupt_handler(1, exception1_handler)
        .register_interrupt_handler(2, exception2_handler)
        .register_user_interrupt_handler(3, exception3_handler)
        .register_user_interrupt_handler(4, exception4_handler)
        .register_interrupt_handler(5, exception5_handler)
        .register_interrupt_handler(6, exception6_handler)
        .register_interrupt_handler(7, exception7_handler)
        .register_interrupt_handler(8, exception8_handler)
        .register_interrupt_handler(9, exception9_handler)
        .register_interrupt_handler(10, exception10_handler)
        .register_interrupt_handler(11, exception11_handler)
        .register_interrupt_handler(12, exception12_handler)
        .register_interrupt_handler(13, exception13_handler)
        .register_interrupt_handler(14, exception14_handler)
        .register_interrupt_handler(16, exception16_handler)
        .register_interrupt_handler(17, exception17_handler)
        .register_interrupt_handler(18, exception18_handler)
        .register_interrupt_handler(19, exception19_handler)
        .register_interrupt_handler(20, exception20_handler)
        .register_interrupt_handler(30, exception30_handler);
}
}
