%ifdef ULTRA_32

section .init
    pop ebp
    ret

section .fini
    pop ebp
    ret

%elifdef ULTRA_64

section .init
    pop rax
    ret

section .fini
    pop rax
    ret

%else

%error Unknown architecture

%endif

