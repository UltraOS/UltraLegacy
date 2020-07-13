#pragma once

#include "Common.h"
#include "Common/Types.h"

namespace kernel {

class IRQHandler;

class IRQManager {
public:
    static constexpr u16 entry_count    = 16;
    static constexpr u16 irq_base_index = 32;

    static void install();

    static void register_irq_handler(IRQHandler&);
    static void unregister_irq_handler(IRQHandler&);

private:
    static bool has_subscriber(u16 request_number);

    static void irq_handler(u16 request_number, RegisterState*) USED;

private:
    static IRQHandler* m_handlers[entry_count];
};
}

// Super specific mangling rules going on here, be careful :)
#define IRQ(handler, len) "_ZN6kernel10IRQManager" TO_STRING(len) TO_STRING(handler) "EtPNS_13RegisterStateE"

// clang-format off

#define DEFINE_IRQ_HANDLER(index)                                                                                      \
    extern "C" void irq##index##_handler();                                                                            \
    asm(".globl irq" #index "_handler\n"                                                                               \
        "irq" #index "_handler:\n"                                                                                     \
        "    pushl $0\n"                                                                                               \
        "    pusha\n"                                                                                                  \
        "    pushl %ds\n"                                                                                              \
        "    pushl %es\n"                                                                                              \
        "    pushl %fs\n"                                                                                              \
        "    pushl %gs\n"                                                                                              \
        "    pushl %ss\n"                                                                                              \
        "    pushl %esp\n"                                                                                             \
        "    pushw $" #index "\n"                                                                                      \
        "    mov $0x10, %ax\n"                                                                                         \
        "    mov %ax, %ds\n"                                                                                           \
        "    mov %ax, %es\n"                                                                                           \
        "    cld\n"                                                                                                    \
        "    call " IRQ(irq_handler, 11) "\n"                                                                          \
        "    add $0xA, %esp \n"                                                                                        \
        "    popl %gs\n"                                                                                               \
        "    popl %fs\n"                                                                                               \
        "    popl %es\n"                                                                                               \
        "    popl %ds\n"                                                                                               \
        "    popa\n"                                                                                                   \
        "    add $0x4, %esp\n"                                                                                         \
        "    iret\n");

// clang-format on
