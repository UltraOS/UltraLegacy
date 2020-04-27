section .init
global _init

_init:
    push ebp
    mov ebp, esp

; ctors ^v dtors

section .fini
global _fini

_fini:
    push ebp
    mov ebp, esp