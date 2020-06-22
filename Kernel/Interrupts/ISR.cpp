#include "Common/Macros.h"
#include "Common/Types.h"
#include "Common/Logger.h"

#include "Common.h"
#include "IDT.h"
#include "ISR.h"

#include "Memory/MemoryManager.h"

namespace kernel {

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(division_by_zero, 24)
    void ISR::division_by_zero_handler(RegisterState)
    {
        error() << "Division by zero!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(debug, 13)
    void ISR::debug_handler(RegisterState)
    {
        error() << "Debug handler!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(non_maskable, 20)
    void ISR::non_maskable_handler(RegisterState)
    {
        error() << "Non maskable interrupt!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(breakpoint, 18)
    void ISR::breakpoint_handler(RegisterState)
    {
        error() << "Breakpoint interrupt!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(overflow, 16)
    void ISR::overflow_handler(RegisterState)
    {
        error() << "Overflow interrupt!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(bound_range_exceeded, 28)
    void ISR::bound_range_exceeded_handler(RegisterState)
    {
        error() << "Bound range exceeded!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(invalid_opcode, 22)
    void ISR::invalid_opcode_handler(RegisterState)
    {
        error() << "Invalid OPcode!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(device_not_available, 28)
    void ISR::device_not_available_handler(RegisterState)
    {
        error() << "Device not available!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(double_fault, 20)
    void ISR::double_fault_handler(RegisterState)
    {
        error() << "Double fault!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(coprocessor_segment_overrun, 35)
    void ISR::coprocessor_segment_overrun_handler(RegisterState)
    {
        error() << "Coprocessor segment overrun! (really?)";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(invalid_tss, 19)
    void ISR::invalid_tss_handler(RegisterState)
    {
        error() << "Invalid TSS!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(segment_not_present, 27)
    void ISR::segment_not_present_handler(RegisterState)
    {
        error() << "Segment not present!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(stack_segment_fault, 27)
    void ISR::stack_segment_fault_handler(RegisterState)
    {
        error() << "Stack segment fault!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(general_protection_fault, 32)
    void ISR::general_protection_fault_handler(RegisterState)
    {
        error() << "General protection fault!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(page_fault, 18)
    void ISR::page_fault_handler(RegisterState state)
    {
        static constexpr u32 type_mask = 0b011;
        static constexpr u32 user_mask = 0b100;

        ptr_t address_of_fault;
        asm("movl %%cr2, %%eax" : "=a"(address_of_fault));

        PageFault pf(address_of_fault, state.error_code & user_mask,
                     static_cast<PageFault::Type>(state.error_code & type_mask));

        MemoryManager::handle_page_fault(pf);

        log() << "PageFaultHandler: page fault resolved, continuing...";
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(floating_point_exception, 32)
    void ISR::floating_point_exception_handler(RegisterState)
    {
        error() << "Floating point exception!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(alignment_check_exception, 33)
    void ISR::alignment_check_exception_handler(RegisterState)
    {
        error() << "Alignment Check exception!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(machine_check_exception, 31)
    void ISR::machine_check_exception_handler(RegisterState)
    {
        error() << "Machine check exception!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(simd_floating_point_exception, 37)
    void ISR::simd_floating_point_exception_handler(RegisterState)
    {
        error() << "SIMD floating point exception!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(virtualization_exception, 32)
    void ISR::virtualization_exception_handler(RegisterState)
    {
        error() << "Virtualization exception!";

        hang();
    }

    DEFINE_INTERRUPT_HANDLER(security_exception, 26)
    void ISR::security_exception_handler(RegisterState)
    {
        error() << "Security exception!";

        hang();
    }

    void ISR::install()
    {
        IDT::the().register_interrupt_handler(division_by_zero_index, division_by_zero_entry)
                  .register_user_interrupt_handler(debug_index, debug_entry)
                  .register_interrupt_handler(non_maskable_index, non_maskable_entry)
                  .register_user_interrupt_handler(breakpoint_index, breakpoint_entry)
                  .register_user_interrupt_handler(overflow_index, overflow_entry)
                  .register_interrupt_handler(bound_range_exceeded_index, bound_range_exceeded_entry)
                  .register_interrupt_handler(invalid_opcode_index, invalid_opcode_entry)
                  .register_interrupt_handler(device_not_available_index, device_not_available_entry)
                  .register_interrupt_handler(double_fault_index, double_fault_entry)
                  .register_interrupt_handler(coprocessor_segment_overrun_index, coprocessor_segment_overrun_entry)
                  .register_interrupt_handler(invalid_tss_index, invalid_opcode_entry)
                  .register_interrupt_handler(segment_not_present_index, segment_not_present_entry)
                  .register_interrupt_handler(stack_segment_fault_index, stack_segment_fault_entry)
                  .register_interrupt_handler(general_protection_fault_index, general_protection_fault_entry)
                  .register_interrupt_handler(page_fault_index, page_fault_entry)
                  .register_interrupt_handler(floating_point_exception_index, floating_point_exception_entry)
                  .register_interrupt_handler(alignment_check_exception_index, alignment_check_exception_entry)
                  .register_interrupt_handler(machine_check_exception_index, machine_check_exception_entry)
                  .register_interrupt_handler(simd_floating_point_exception_index, simd_floating_point_exception_entry)
                  .register_interrupt_handler(virtualization_exception_index, virtualization_exception_entry)
                  .register_interrupt_handler(security_exception_index, security_exception_entry);
    }
}
