extern libc_entrypoint

global _start
_start:
%ifdef ULTRA_64
    mov rdi, rsp
%elifdef ULTRA_32
    push 0
    push esp
    add [esp], dword 4
%else
    %error Unknown architecture
%endif
    call libc_entrypoint
    ud2
