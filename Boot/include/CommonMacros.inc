%ifndef _COMMON_MACROS_INC_
%define _COMMON_MACROS_INC_

CR: equ 0x0D
LF: equ 0x0A

; magic breakpoint in Bochs
%define BP xchg bx, bx

%endif ; _COMMON_MACROS_INC_
