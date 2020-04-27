extern _init
extern _fini
extern run

section .entry

global start
start:
    ; Set up the kernel stack
    mov esp, kernel_stack_begin

    ; Call all global constructors
    call _init

    ; Jump into kernel main
    call run

    ; call global destructors
    call _fini
hang:
    cli
    hlt
    jmp hang

align 16
kernel_stack_end:
    times 16384 db 0
kernel_stack_begin:
