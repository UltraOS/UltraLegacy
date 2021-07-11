%ifdef ULTRA_32

section .init
global _init
_init:
    push ebp

section .fini
global _fini
_fini:
    push ebp

%elifdef ULTRA_64

section .init
global _init
_init:
    push rax

section .fini
global _fini
_fini:
    push rax

%else

%error Unknown architecture

%endif

