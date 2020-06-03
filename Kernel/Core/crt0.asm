%define KERNEL_ENTRYPOINT _ZN6kernel3runENS_9MemoryMapE

extern KERNEL_ENTRYPOINT

section .entry

global start
start:
    ; Set up the kernel stack
    mov esp, kernel_stack_begin

    ; entry count
    push ebx
    ; memory map pointer
    push eax

    ; Jump into kernel main
    call KERNEL_ENTRYPOINT
hang:
    cli
    hlt
    jmp hang

align 16
kernel_stack_end:
    times 16384 db 0
kernel_stack_begin:

section .magic
    global magic_string
    ; a magic "CRC" string to verify the integrity
    magic_string: db "MAGIC_PRESENT", 0
