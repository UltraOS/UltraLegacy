#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

#include "Common.h"

#define EXCEPTION_HANDLER_SYMBOL(index) extern "C" void exception##index##_handler()

namespace kernel {

class ExceptionHandler;

class ExceptionDispatcher {
public:
    static constexpr size_t exception_count = 20;

    static void install();

    static void register_handler(ExceptionHandler&);
    static void unregister_handler(ExceptionHandler&);

private:
    static void exception_handler(RegisterState*) USED;

    static const constexpr StringView s_exception_messages[] = { "Division-by-zero"_sv,
                                                                 "Debug"_sv,
                                                                 "Non-maskable interrupt"_sv,
                                                                 "Breakpoint"_sv,
                                                                 "Overflow"_sv,
                                                                 "Bound Range Exceeded"_sv,
                                                                 "Invalid Opcode"_sv,
                                                                 "Device Not Available"_sv,
                                                                 "Double Fault"_sv,
                                                                 "Coprocessor Segment Overrun"_sv,
                                                                 "Invalid TSS"_sv,
                                                                 "Segment Not Present"_sv,
                                                                 "Stack-Segment Fault"_sv,
                                                                 "General Protection Fault"_sv,
                                                                 "Page Fault"_sv,
                                                                 "Reserved"_sv,
                                                                 "Floating-Point"_sv,
                                                                 "Alignment Check"_sv,
                                                                 "Machine Check"_sv,
                                                                 "SIMD Floating-Point"_sv,
                                                                 "Virtualization"_sv,
                                                                 "Security Exception"_sv };

private:
    static ExceptionHandler* s_handlers[exception_count];
};
}
