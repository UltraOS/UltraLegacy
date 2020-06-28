#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

#include "Common.h"

namespace kernel {

class ISR {
public:
    static void install();

private:
    static void division_by_zero_handler(RegisterState) USED;
    static void debug_handler(RegisterState) USED;
    static void non_maskable_handler(RegisterState) USED;
    static void breakpoint_handler(RegisterState) USED;
    static void overflow_handler(RegisterState) USED;
    static void bound_range_exceeded_handler(RegisterState) USED;
    static void invalid_opcode_handler(RegisterState) USED;
    static void device_not_available_handler(RegisterState) USED;
    static void double_fault_handler(RegisterState) USED;
    static void coprocessor_segment_overrun_handler(RegisterState) USED;
    static void invalid_tss_handler(RegisterState) USED;
    static void segment_not_present_handler(RegisterState) USED;
    static void stack_segment_fault_handler(RegisterState) USED;
    static void general_protection_fault_handler(RegisterState) USED;
    static void page_fault_handler(RegisterState) USED;
    static void floating_point_exception_handler(RegisterState) USED;
    static void alignment_check_exception_handler(RegisterState) USED;
    static void machine_check_exception_handler(RegisterState) USED;
    static void simd_floating_point_exception_handler(RegisterState) USED;
    static void virtualization_exception_handler(RegisterState) USED;
    static void security_exception_handler(RegisterState) USED;

private:
    static constexpr u16 division_by_zero_index              = 0;
    static constexpr u16 debug_index                         = 1;
    static constexpr u16 non_maskable_index                  = 2;
    static constexpr u16 breakpoint_index                    = 3;
    static constexpr u16 overflow_index                      = 4;
    static constexpr u16 bound_range_exceeded_index          = 5;
    static constexpr u16 invalid_opcode_index                = 6;
    static constexpr u16 device_not_available_index          = 7;
    static constexpr u16 double_fault_index                  = 8;
    static constexpr u16 coprocessor_segment_overrun_index   = 9;
    static constexpr u16 invalid_tss_index                   = 10;
    static constexpr u16 segment_not_present_index           = 11;
    static constexpr u16 stack_segment_fault_index           = 12;
    static constexpr u16 general_protection_fault_index      = 13;
    static constexpr u16 page_fault_index                    = 14;
    static constexpr u16 floating_point_exception_index      = 16;
    static constexpr u16 alignment_check_exception_index     = 17;
    static constexpr u16 machine_check_exception_index       = 18;
    static constexpr u16 simd_floating_point_exception_index = 19;
    static constexpr u16 virtualization_exception_index      = 20;
    static constexpr u16 security_exception_index            = 30;
};
}

// Super specific mangling rules going on here, be careful :)
#define ISR(handler, len) "_ZN6kernel3ISR" TO_STRING(len) TO_STRING(handler) "ENS_13RegisterStateE"

// Please stop...
// clang-format off

// title - the interrupt handler title without "_handler"
// length - the full length of the actual handler, including "_handler"
// (and yes there isn't any other way to get the length as a literal AFAIK)
#define DEFINE_INTERRUPT_HANDLER(title, length)                                                                        \
    extern "C" void title##_entry();                                                                                   \
    asm(".globl " #title "_entry\n"                                                                                    \
        "" #title "_entry: \n"                                                                                         \
        "    pusha\n"                                                                                                  \
        "    pushl %ds\n"                                                                                              \
        "    pushl %es\n"                                                                                              \
        "    pushl %fs\n"                                                                                              \
        "    pushl %gs\n"                                                                                              \
        "    pushl %ss\n"                                                                                              \
        "    mov $0x10, %ax\n"                                                                                         \
        "    mov %ax, %ds\n"                                                                                           \
        "    mov %ax, %es\n"                                                                                           \
        "    cld\n"                                                                                                    \
        "    call " ISR(title##_handler, length) "\n"                                                                  \
        "    add $0x4, %esp \n"                                                                                        \
        "    popl %gs\n"                                                                                               \
        "    popl %fs\n"                                                                                               \
        "    popl %es\n"                                                                                               \
        "    popl %ds\n"                                                                                               \
        "    popa\n"                                                                                                   \
        "    add $0x4, %esp\n"                                                                                         \
        "    iret\n");

// title - the interrupt handler title without "_handler"
// length - the full length of the actual handler, including "_handler"
// (and yes there isn't any other way to get the length as a literal AFAIK)
#define DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(title, length)                                                          \
    extern "C" void title##_entry();                                                                                   \
    asm(".globl " #title "_entry\n"                                                                                    \
        "" #title "_entry: \n"                                                                                         \
        "    pushl $0\n"                                                                                               \
        "    pusha\n"                                                                                                  \
        "    pushl %ds\n"                                                                                              \
        "    pushl %es\n"                                                                                              \
        "    pushl %fs\n"                                                                                              \
        "    pushl %gs\n"                                                                                              \
        "    pushl %ss\n"                                                                                              \
        "    mov $0x10, %ax\n"                                                                                         \
        "    mov %ax, %ds\n"                                                                                           \
        "    mov %ax, %es\n"                                                                                           \
        "    cld\n"                                                                                                    \
        "    call " ISR(title##_handler, length) "\n"                                                                  \
        "    add $0x4, %esp \n"                                                                                        \
        "    popl %gs\n"                                                                                               \
        "    popl %fs\n"                                                                                               \
        "    popl %es\n"                                                                                               \
        "    popl %ds\n"                                                                                               \
        "    popa\n"                                                                                                   \
        "    add $0x4, %esp\n"                                                                                         \
        "    iret\n");

// clang-format on
