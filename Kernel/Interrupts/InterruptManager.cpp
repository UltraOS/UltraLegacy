#include "InterruptManager.h"
#include "ExceptionDispatcher.h"
#include "IDT.h"
#include "IPICommunicator.h"
#include "IVectorAllocator.h"
#include "SyscallDispatcher.h"
#include "IRQManager.h"
#include "Common/Logger.h"

namespace kernel {

InterruptHandler* InterruptManager::s_handlers[IDT::entry_count];

void InterruptManager::handle_interrupt(RegisterState* registers)
{
    auto interrupt_number = registers->interrupt_number;

    if (interrupt_number >= IDT::entry_count) {
        String error_str;
        error_str << "InterruptManager: invalid interrupt number " << interrupt_number;
        runtime::panic(error_str.c_string());
    }

    auto* handler = s_handlers[interrupt_number];

    if (handler == nullptr) {
        String error_str;
        error_str << "InterruptManager: unhandled interrupt " << interrupt_number;
        runtime::panic(error_str.c_string());
    }

    handler->handle_interrupt(*registers);
}

void InterruptManager::set_handler_for_vector(u16 vector, InterruptHandler& handler, bool user_callable)
{
    if (vector >= IDT::entry_count) {
        String error_str;
        error_str << "InterruptManager: tried to set handler for invalid interrupt number " << vector;
        runtime::panic(error_str.c_string());
    }

    if (s_handlers[vector])
        runtime::panic("InterruptHandler: tried to register a handler for an already managed interrupt!");

    s_handlers[vector] = &handler;

    if (user_callable)
        IDT::the().make_user_callable(vector);
}

void InterruptManager::remove_handler_for_vector(u16 vector, InterruptHandler& owner)
{
    if (vector >= IDT::entry_count) {
        String error_str;
        error_str << "InterruptManager: tried to unregsiter invalid interrupt number " << vector;
        runtime::panic(error_str.c_string());
    }

    if (s_handlers[vector] != &owner)
        runtime::panic("InterruptHandler: tried to unregister a non-registered handler!");

    s_handlers[vector] = nullptr;
    IDT::the().make_non_user_callable(vector);
}

void InterruptManager::register_handler(InterruptHandler& handler)
{
    switch (handler.interrupt_handler_type()) {
    case InterruptHandler::Type::MONO: {
        auto& mono = static_cast<MonoInterruptHandler&>(handler);

        set_handler_for_vector(mono.interrupt_vector(), handler, mono.is_user_callable());
        break;
    }
    case InterruptHandler::Type::RANGED: {
        auto& ranged = static_cast<RangedInterruptHandler&>(handler);
        auto range = ranged.managed_range();

        for (size_t i = range.begin(); i < range.end(); ++i)
            set_handler_for_vector(i, handler, false);

        for (auto user_vector : ranged.user_vectors())
            IDT::the().make_user_callable(user_vector);
        break;
    }
    default:
        ASSERT_NEVER_REACHED();
    }
}

void InterruptManager::unregister_handler(InterruptHandler& handler)
{
    switch (handler.interrupt_handler_type()) {
    case InterruptHandler::Type::MONO: {
        auto& mono = static_cast<MonoInterruptHandler&>(handler);

        remove_handler_for_vector(mono.interrupt_vector(), handler);
        break;
    }
    case InterruptHandler::Type::RANGED: {
        auto& ranged = static_cast<RangedInterruptHandler&>(handler);
        auto range = ranged.managed_range();

        for (size_t i = range.begin(); i < range.end(); ++i)
            remove_handler_for_vector(i, handler);

        break;
    }
    default:
        ASSERT_NEVER_REACHED();
    }
}

void InterruptManager::register_dynamic_handler(DynamicInterruptHandler& handler, u16 vector, bool user_callable)
{
    set_handler_for_vector(vector, handler, user_callable);
}

void InterruptManager::unregister_dynamic_handler(DynamicInterruptHandler& handler, u16 vector)
{
    remove_handler_for_vector(vector, handler);
}

#define INTERRUPT_HANDLER_SYMBOL(index) extern "C" void interrupt##index##_handler()

#define ENUMERATE_INTERRUPTS \
    INTERRUPT_HANDLER(0)     \
    INTERRUPT_HANDLER(1)     \
    INTERRUPT_HANDLER(2)     \
    INTERRUPT_HANDLER(3)     \
    INTERRUPT_HANDLER(4)     \
    INTERRUPT_HANDLER(5)     \
    INTERRUPT_HANDLER(6)     \
    INTERRUPT_HANDLER(7)     \
    INTERRUPT_HANDLER(8)     \
    INTERRUPT_HANDLER(9)     \
    INTERRUPT_HANDLER(10)    \
    INTERRUPT_HANDLER(11)    \
    INTERRUPT_HANDLER(12)    \
    INTERRUPT_HANDLER(13)    \
    INTERRUPT_HANDLER(14)    \
    INTERRUPT_HANDLER(15)    \
    INTERRUPT_HANDLER(16)    \
    INTERRUPT_HANDLER(17)    \
    INTERRUPT_HANDLER(18)    \
    INTERRUPT_HANDLER(19)    \
    INTERRUPT_HANDLER(20)    \
    INTERRUPT_HANDLER(21)    \
    INTERRUPT_HANDLER(22)    \
    INTERRUPT_HANDLER(23)    \
    INTERRUPT_HANDLER(24)    \
    INTERRUPT_HANDLER(25)    \
    INTERRUPT_HANDLER(26)    \
    INTERRUPT_HANDLER(27)    \
    INTERRUPT_HANDLER(28)    \
    INTERRUPT_HANDLER(29)    \
    INTERRUPT_HANDLER(30)    \
    INTERRUPT_HANDLER(31)    \
    INTERRUPT_HANDLER(32)    \
    INTERRUPT_HANDLER(33)    \
    INTERRUPT_HANDLER(34)    \
    INTERRUPT_HANDLER(35)    \
    INTERRUPT_HANDLER(36)    \
    INTERRUPT_HANDLER(37)    \
    INTERRUPT_HANDLER(38)    \
    INTERRUPT_HANDLER(39)    \
    INTERRUPT_HANDLER(40)    \
    INTERRUPT_HANDLER(41)    \
    INTERRUPT_HANDLER(42)    \
    INTERRUPT_HANDLER(43)    \
    INTERRUPT_HANDLER(44)    \
    INTERRUPT_HANDLER(45)    \
    INTERRUPT_HANDLER(46)    \
    INTERRUPT_HANDLER(47)    \
    INTERRUPT_HANDLER(48)    \
    INTERRUPT_HANDLER(49)    \
    INTERRUPT_HANDLER(50)    \
    INTERRUPT_HANDLER(51)    \
    INTERRUPT_HANDLER(52)    \
    INTERRUPT_HANDLER(53)    \
    INTERRUPT_HANDLER(54)    \
    INTERRUPT_HANDLER(55)    \
    INTERRUPT_HANDLER(56)    \
    INTERRUPT_HANDLER(57)    \
    INTERRUPT_HANDLER(58)    \
    INTERRUPT_HANDLER(59)    \
    INTERRUPT_HANDLER(60)    \
    INTERRUPT_HANDLER(61)    \
    INTERRUPT_HANDLER(62)    \
    INTERRUPT_HANDLER(63)    \
    INTERRUPT_HANDLER(64)    \
    INTERRUPT_HANDLER(65)    \
    INTERRUPT_HANDLER(66)    \
    INTERRUPT_HANDLER(67)    \
    INTERRUPT_HANDLER(68)    \
    INTERRUPT_HANDLER(69)    \
    INTERRUPT_HANDLER(70)    \
    INTERRUPT_HANDLER(71)    \
    INTERRUPT_HANDLER(72)    \
    INTERRUPT_HANDLER(73)    \
    INTERRUPT_HANDLER(74)    \
    INTERRUPT_HANDLER(75)    \
    INTERRUPT_HANDLER(76)    \
    INTERRUPT_HANDLER(77)    \
    INTERRUPT_HANDLER(78)    \
    INTERRUPT_HANDLER(79)    \
    INTERRUPT_HANDLER(80)    \
    INTERRUPT_HANDLER(81)    \
    INTERRUPT_HANDLER(82)    \
    INTERRUPT_HANDLER(83)    \
    INTERRUPT_HANDLER(84)    \
    INTERRUPT_HANDLER(85)    \
    INTERRUPT_HANDLER(86)    \
    INTERRUPT_HANDLER(87)    \
    INTERRUPT_HANDLER(88)    \
    INTERRUPT_HANDLER(89)    \
    INTERRUPT_HANDLER(90)    \
    INTERRUPT_HANDLER(91)    \
    INTERRUPT_HANDLER(92)    \
    INTERRUPT_HANDLER(93)    \
    INTERRUPT_HANDLER(94)    \
    INTERRUPT_HANDLER(95)    \
    INTERRUPT_HANDLER(96)    \
    INTERRUPT_HANDLER(97)    \
    INTERRUPT_HANDLER(98)    \
    INTERRUPT_HANDLER(99)    \
    INTERRUPT_HANDLER(100)   \
    INTERRUPT_HANDLER(101)   \
    INTERRUPT_HANDLER(102)   \
    INTERRUPT_HANDLER(103)   \
    INTERRUPT_HANDLER(104)   \
    INTERRUPT_HANDLER(105)   \
    INTERRUPT_HANDLER(106)   \
    INTERRUPT_HANDLER(107)   \
    INTERRUPT_HANDLER(108)   \
    INTERRUPT_HANDLER(109)   \
    INTERRUPT_HANDLER(110)   \
    INTERRUPT_HANDLER(111)   \
    INTERRUPT_HANDLER(112)   \
    INTERRUPT_HANDLER(113)   \
    INTERRUPT_HANDLER(114)   \
    INTERRUPT_HANDLER(115)   \
    INTERRUPT_HANDLER(116)   \
    INTERRUPT_HANDLER(117)   \
    INTERRUPT_HANDLER(118)   \
    INTERRUPT_HANDLER(119)   \
    INTERRUPT_HANDLER(120)   \
    INTERRUPT_HANDLER(121)   \
    INTERRUPT_HANDLER(122)   \
    INTERRUPT_HANDLER(123)   \
    INTERRUPT_HANDLER(124)   \
    INTERRUPT_HANDLER(125)   \
    INTERRUPT_HANDLER(126)   \
    INTERRUPT_HANDLER(127)   \
    INTERRUPT_HANDLER(128)   \
    INTERRUPT_HANDLER(129)   \
    INTERRUPT_HANDLER(130)   \
    INTERRUPT_HANDLER(131)   \
    INTERRUPT_HANDLER(132)   \
    INTERRUPT_HANDLER(133)   \
    INTERRUPT_HANDLER(134)   \
    INTERRUPT_HANDLER(135)   \
    INTERRUPT_HANDLER(136)   \
    INTERRUPT_HANDLER(137)   \
    INTERRUPT_HANDLER(138)   \
    INTERRUPT_HANDLER(139)   \
    INTERRUPT_HANDLER(141)   \
    INTERRUPT_HANDLER(142)   \
    INTERRUPT_HANDLER(143)   \
    INTERRUPT_HANDLER(144)   \
    INTERRUPT_HANDLER(145)   \
    INTERRUPT_HANDLER(146)   \
    INTERRUPT_HANDLER(147)   \
    INTERRUPT_HANDLER(148)   \
    INTERRUPT_HANDLER(149)   \
    INTERRUPT_HANDLER(150)   \
    INTERRUPT_HANDLER(151)   \
    INTERRUPT_HANDLER(152)   \
    INTERRUPT_HANDLER(153)   \
    INTERRUPT_HANDLER(154)   \
    INTERRUPT_HANDLER(155)   \
    INTERRUPT_HANDLER(156)   \
    INTERRUPT_HANDLER(157)   \
    INTERRUPT_HANDLER(158)   \
    INTERRUPT_HANDLER(159)   \
    INTERRUPT_HANDLER(160)   \
    INTERRUPT_HANDLER(161)   \
    INTERRUPT_HANDLER(162)   \
    INTERRUPT_HANDLER(163)   \
    INTERRUPT_HANDLER(164)   \
    INTERRUPT_HANDLER(165)   \
    INTERRUPT_HANDLER(166)   \
    INTERRUPT_HANDLER(167)   \
    INTERRUPT_HANDLER(168)   \
    INTERRUPT_HANDLER(169)   \
    INTERRUPT_HANDLER(170)   \
    INTERRUPT_HANDLER(171)   \
    INTERRUPT_HANDLER(172)   \
    INTERRUPT_HANDLER(173)   \
    INTERRUPT_HANDLER(174)   \
    INTERRUPT_HANDLER(175)   \
    INTERRUPT_HANDLER(176)   \
    INTERRUPT_HANDLER(177)   \
    INTERRUPT_HANDLER(178)   \
    INTERRUPT_HANDLER(179)   \
    INTERRUPT_HANDLER(180)   \
    INTERRUPT_HANDLER(181)   \
    INTERRUPT_HANDLER(182)   \
    INTERRUPT_HANDLER(183)   \
    INTERRUPT_HANDLER(184)   \
    INTERRUPT_HANDLER(185)   \
    INTERRUPT_HANDLER(186)   \
    INTERRUPT_HANDLER(187)   \
    INTERRUPT_HANDLER(188)   \
    INTERRUPT_HANDLER(189)   \
    INTERRUPT_HANDLER(190)   \
    INTERRUPT_HANDLER(191)   \
    INTERRUPT_HANDLER(192)   \
    INTERRUPT_HANDLER(193)   \
    INTERRUPT_HANDLER(194)   \
    INTERRUPT_HANDLER(195)   \
    INTERRUPT_HANDLER(196)   \
    INTERRUPT_HANDLER(197)   \
    INTERRUPT_HANDLER(198)   \
    INTERRUPT_HANDLER(199)   \
    INTERRUPT_HANDLER(200)   \
    INTERRUPT_HANDLER(201)   \
    INTERRUPT_HANDLER(202)   \
    INTERRUPT_HANDLER(203)   \
    INTERRUPT_HANDLER(204)   \
    INTERRUPT_HANDLER(205)   \
    INTERRUPT_HANDLER(206)   \
    INTERRUPT_HANDLER(207)   \
    INTERRUPT_HANDLER(208)   \
    INTERRUPT_HANDLER(209)   \
    INTERRUPT_HANDLER(210)   \
    INTERRUPT_HANDLER(211)   \
    INTERRUPT_HANDLER(212)   \
    INTERRUPT_HANDLER(213)   \
    INTERRUPT_HANDLER(214)   \
    INTERRUPT_HANDLER(215)   \
    INTERRUPT_HANDLER(216)   \
    INTERRUPT_HANDLER(217)   \
    INTERRUPT_HANDLER(218)   \
    INTERRUPT_HANDLER(219)   \
    INTERRUPT_HANDLER(220)   \
    INTERRUPT_HANDLER(221)   \
    INTERRUPT_HANDLER(222)   \
    INTERRUPT_HANDLER(223)   \
    INTERRUPT_HANDLER(224)   \
    INTERRUPT_HANDLER(225)   \
    INTERRUPT_HANDLER(226)   \
    INTERRUPT_HANDLER(227)   \
    INTERRUPT_HANDLER(228)   \
    INTERRUPT_HANDLER(229)   \
    INTERRUPT_HANDLER(230)   \
    INTERRUPT_HANDLER(231)   \
    INTERRUPT_HANDLER(232)   \
    INTERRUPT_HANDLER(233)   \
    INTERRUPT_HANDLER(234)   \
    INTERRUPT_HANDLER(235)   \
    INTERRUPT_HANDLER(236)   \
    INTERRUPT_HANDLER(237)   \
    INTERRUPT_HANDLER(238)   \
    INTERRUPT_HANDLER(239)   \
    INTERRUPT_HANDLER(240)   \
    INTERRUPT_HANDLER(241)   \
    INTERRUPT_HANDLER(242)   \
    INTERRUPT_HANDLER(243)   \
    INTERRUPT_HANDLER(244)   \
    INTERRUPT_HANDLER(245)   \
    INTERRUPT_HANDLER(246)   \
    INTERRUPT_HANDLER(247)   \
    INTERRUPT_HANDLER(248)   \
    INTERRUPT_HANDLER(249)   \
    INTERRUPT_HANDLER(250)   \
    INTERRUPT_HANDLER(251)   \
    INTERRUPT_HANDLER(252)   \
    INTERRUPT_HANDLER(253)   \
    INTERRUPT_HANDLER(254)   \
    INTERRUPT_HANDLER(255)

#define INTERRUPT_HANDLER(index) extern "C" void interrupt##index##_handler();
ENUMERATE_INTERRUPTS
#undef INTERRUPT_HANDLER

void InterruptManager::initialize_all()
{
#define INTERRUPT_HANDLER(index) .register_interrupt_handler(index, &interrupt##index##_handler)

    IDT::the()
        ENUMERATE_INTERRUPTS;

    IVectorAllocator::initialize();
    ExceptionDispatcher::initialize();
    SyscallDispatcher::initialize();
    IPICommunicator::initialize();
    IRQManager::initialize();
}

}
