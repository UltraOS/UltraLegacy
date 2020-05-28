#include "InterruptDescriptorTable.h"
#include "InterruptServiceRoutines.h"
#include "Macros.h"
#include "Log.h"

namespace kernel {
    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(division_by_zero, 24)
    void InterruptServiceRoutines::division_by_zero_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Division by zero!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(debug, 13)
    void InterruptServiceRoutines::debug_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Debug handler!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(non_maskable, 20)
    void InterruptServiceRoutines::non_maskable_handler(RegisterState registers)
    {
        (void) registers;
        
        error() << "Non maskable interrupt!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(breakpoint, 18)
    void InterruptServiceRoutines::breakpoint_handler(RegisterState registers)
    {
        (void) registers;
        
        error() << "Breakpoint interrupt!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(overflow, 16)
    void InterruptServiceRoutines::overflow_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Overflow interrupt!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(bound_range_exceeded, 28)
    void InterruptServiceRoutines::bound_range_exceeded_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Bound range exceeded!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(invalid_opcode, 22)
    void InterruptServiceRoutines::invalid_opcode_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Invalid OPcode!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(device_not_available, 28)
    void InterruptServiceRoutines::device_not_available_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Device not available!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(double_fault, 20)
    void InterruptServiceRoutines::double_fault_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Double fault!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(coprocessor_segment_overrun, 35)
    void InterruptServiceRoutines::coprocessor_segment_overrun_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Coprocessor segment overrun! (really?)";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(invalid_tss, 19)
    void InterruptServiceRoutines::invalid_tss_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Invalid TSS!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(segment_not_present, 27)
    void InterruptServiceRoutines::segment_not_present_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Segment not present!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(stack_segment_fault, 27)
    void InterruptServiceRoutines::stack_segment_fault_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Stack segment fault!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(general_protection_fault, 32)
    void InterruptServiceRoutines::general_protection_fault_handler(RegisterState registers)
    {
        (void) registers;

        error() << "General protection fault!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(pagefault, 17)
    void InterruptServiceRoutines::pagefault_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Pagefault!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(floating_point_exception, 32)
    void InterruptServiceRoutines::floating_point_exception_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Floating point exception!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(alignment_check_exception, 33)
    void InterruptServiceRoutines::alignment_check_exception_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Alignment Check exception!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(machine_check_exception, 31)
    void InterruptServiceRoutines::machine_check_exception_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Machine check exception!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(simd_floating_point_exception, 37)
    void InterruptServiceRoutines::simd_floating_point_exception_handler(RegisterState registers)
    {
        (void) registers;

        error() << "SIMD floating point exception!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER_NO_ERROR_CODE(virtualization_exception, 32)
    void InterruptServiceRoutines::virtualization_exception_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Virtualization exception!";

        for(;;);
    }

    DEFINE_INTERRUPT_HANDLER(security_exception, 26)
    void InterruptServiceRoutines::security_exception_handler(RegisterState registers)
    {
        (void) registers;

        error() << "Security exception!";

        for(;;);
    }

    void InterruptServiceRoutines::install()
    {
        InterruptDescriptorTable::the()
            .register_interrupt_handler(division_by_zero_index, division_by_zero_entry)
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
            .register_interrupt_handler(pagefault_index, pagefault_entry)
            .register_interrupt_handler(floating_point_exception_index, floating_point_exception_entry)
            .register_interrupt_handler(alignment_check_exception_index, alignment_check_exception_entry)
            .register_interrupt_handler(machine_check_exception_index, machine_check_exception_entry)
            .register_interrupt_handler(simd_floating_point_exception_index, simd_floating_point_exception_entry)
            .register_interrupt_handler(virtualization_exception_index, virtualization_exception_entry)
            .register_interrupt_handler(security_exception_index, security_exception_entry);
    }
}
