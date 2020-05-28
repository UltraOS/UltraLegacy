extern _ZN6kernel3runEv

section .entry

global start
start:
    ; Set up the kernel stack
    mov esp, kernel_stack_begin

    ; Jump into kernel main
    call _ZN6kernel3runEv
hang:
    cli
    hlt
    jmp hang

align 16
kernel_stack_end:
    times 16384 db 0
kernel_stack_begin:
