%define KERNEL_INTERRUPT_MANAGER _ZN6kernel16InterruptManager16handle_interruptEPNS_13RegisterStateE
extern KERNEL_INTERRUPT_MANAGER

%define INTERRUPT_HANDLER(index) interrupt %+ index %+ _handler

section .text

common_interrupt_handler:
    pusha
    push ds
    push es
    push fs
    push gs
    push ss
    push esp
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    cld
    call KERNEL_INTERRUPT_MANAGER
    add esp, 0x8
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 0x8
    iret

%macro GENERIC_INTERRUPT_HANDLER 1
global INTERRUPT_HANDLER(%1)
INTERRUPT_HANDLER(%1):
    push dword 0  ; error_code
    push dword %1 ; irq_number
    jmp common_interrupt_handler
%endmacro

%macro EXCEPTION_HANDLER 1
global INTERRUPT_HANDLER(%1)
INTERRUPT_HANDLER(%1):
    push dword %1 ; exception_number
    jmp common_interrupt_handler
%endmacro

%macro EXCEPTION_HANDLER_NO_ERROR_CODE 1
global INTERRUPT_HANDLER(%1)
INTERRUPT_HANDLER(%1):
    push dword 0  ; error_code
    push dword %1 ; exception_number
    jmp common_interrupt_handler
%endmacro

%include "Interrupts.inc"
