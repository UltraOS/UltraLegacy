%define KERNEL_IRQ_HANDLER       _ZN6kernel10IRQManager11irq_handlerEPNS_13RegisterStateE
%define KERNEL_EXCEPTION_HANDLER _ZN6kernel19ExceptionDispatcher17exception_handlerEPNS_13RegisterStateE
%define KERNEL_SYSCALL_HANDLER   _ZN6kernel17SyscallDispatcher8dispatchEPNS_13RegisterStateE

extern KERNEL_IRQ_HANDLER
extern KERNEL_EXCEPTION_HANDLER
extern KERNEL_SYSCALL_HANDLER

%include "Common.inc"

section .text
global syscall_handler
syscall_handler:
    push qword 0 ; error_code
    push qword 0x80 ; interrupt number
    pushaq
    mov  rdi, rsp
    cld
    call KERNEL_SYSCALL_HANDLER
    popaq
    add rsp, 0x10
    iretq

%define IRQ_HANDLER_SYMBOL(index) irq %+ index %+ _handler
%define EXCEPTION_HANDLER_SYMBOL(index) exception %+ index %+ _handler

common_irq_handler:
    pushaq
    mov rdi, rsp
    cld
    call KERNEL_IRQ_HANDLER
    popaq
    add rsp, 0x10
    iretq

common_exception_handler:
    pushaq
    mov rdi, rsp
    cld
    call KERNEL_EXCEPTION_HANDLER
    popaq
    add rsp, 0x10
    iretq

%macro IRQ_HANDLER 1
global IRQ_HANDLER_SYMBOL(%1)
IRQ_HANDLER_SYMBOL(%1):
    push qword 0  ; error_code
    push qword %1 ; irq_number
    jmp common_irq_handler
%endmacro

%macro EXCEPTION_HANDLER 1
global EXCEPTION_HANDLER_SYMBOL(%1)
EXCEPTION_HANDLER_SYMBOL(%1):
    push qword %1 ; exception_number
    jmp common_exception_handler
%endmacro

%macro EXCEPTION_HANDLER_NO_ERROR_CODE 1
global EXCEPTION_HANDLER_SYMBOL(%1)
EXCEPTION_HANDLER_SYMBOL(%1):
    push qword 0  ; error_code
    push qword %1 ; exception_number
    jmp common_exception_handler
%endmacro

IRQ_HANDLER 0
IRQ_HANDLER 1
IRQ_HANDLER 2
IRQ_HANDLER 3
IRQ_HANDLER 4
IRQ_HANDLER 5
IRQ_HANDLER 6
IRQ_HANDLER 7
IRQ_HANDLER 8
IRQ_HANDLER 9
IRQ_HANDLER 10
IRQ_HANDLER 11
IRQ_HANDLER 12
IRQ_HANDLER 13
IRQ_HANDLER 14
IRQ_HANDLER 15
IRQ_HANDLER 255

EXCEPTION_HANDLER_NO_ERROR_CODE 0 ; Division-by-zero
EXCEPTION_HANDLER_NO_ERROR_CODE 1 ; Debug
EXCEPTION_HANDLER_NO_ERROR_CODE 2 ; Non-maskable
EXCEPTION_HANDLER_NO_ERROR_CODE 3 ; Breakpoint
EXCEPTION_HANDLER_NO_ERROR_CODE 4 ; Overflow
EXCEPTION_HANDLER_NO_ERROR_CODE 5 ; Bound Range Exceeded
EXCEPTION_HANDLER_NO_ERROR_CODE 6 ; Invalid Opcode
EXCEPTION_HANDLER_NO_ERROR_CODE 7 ; Device Not Available
EXCEPTION_HANDLER_NO_ERROR_CODE 9 ; Coprocessor Segment Overrun
EXCEPTION_HANDLER_NO_ERROR_CODE 16 ; Floating-Point
EXCEPTION_HANDLER_NO_ERROR_CODE 18 ; Machine Check
EXCEPTION_HANDLER_NO_ERROR_CODE 19 ; SIMD Floating-Point
EXCEPTION_HANDLER_NO_ERROR_CODE 20 ; Virtualization

EXCEPTION_HANDLER 8 ; Double Fault
EXCEPTION_HANDLER 10 ; Invalid TSS
EXCEPTION_HANDLER 11 ; Segment Not Present
EXCEPTION_HANDLER 12 ; Stack-Segmen
EXCEPTION_HANDLER 13 ; General Protection Fault
EXCEPTION_HANDLER 14 ; Page Fault
EXCEPTION_HANDLER 17 ; Alignment Check
EXCEPTION_HANDLER 30 ; Security Exception
