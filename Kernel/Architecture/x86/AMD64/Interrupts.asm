%define KERNEL_INTERRUPT_MANAGER _ZN6kernel16InterruptManager16handle_interruptEPNS_13RegisterStateE
extern KERNEL_INTERRUPT_MANAGER

%include "Common.inc"

section .text

%define INTERRUPT_HANDLER(index) interrupt %+ index %+ _handler

common_interrupt_handler:
    pushaq
    mov rdi, rsp
    cld
    call KERNEL_INTERRUPT_MANAGER
    popaq
    add rsp, 0x10
    iretq

%macro GENERIC_INTERRUPT_HANDLER 1
global INTERRUPT_HANDLER(%1)
INTERRUPT_HANDLER(%1):
    push qword 0  ; error_code
    push qword %1 ; irq_number
    jmp common_interrupt_handler
%endmacro

%macro EXCEPTION_HANDLER 1
global INTERRUPT_HANDLER(%1)
INTERRUPT_HANDLER(%1):
    push qword %1 ; exception_number
    jmp common_interrupt_handler
%endmacro

%macro EXCEPTION_HANDLER_NO_ERROR_CODE 1
global INTERRUPT_HANDLER(%1)
INTERRUPT_HANDLER(%1):
    push qword 0  ; error_code
    push qword %1 ; exception_number
    jmp common_interrupt_handler
%endmacro

%include "Interrupts.inc"
