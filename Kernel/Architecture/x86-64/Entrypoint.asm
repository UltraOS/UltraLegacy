%define KERNEL_ENTRYPOINT _ZN6kernel3runEPNS_9MemoryMapE

extern KERNEL_ENTRYPOINT

section .entry

extern section_bss_begin
extern section_bss_end

EM_BIT: equ (1 << 2)
TS_BIT: equ (1 << 3)
VIRTUAL_ORIGIN: equ 0xFFFFFFFF80000000

global start
start:
    mov r8, cr3
    mov qword [r8], 0x0000000000000000
    mov cr3, r8

    ; Set up the kernel stack
    mov rsp, kernel_stack_begin

    ; entry count
    mov r8, rbx

    ; memory map pointer
    mov r9, rax
    add r9, VIRTUAL_ORIGIN

    ; zero section bss
    mov rcx, section_bss_end
    sub rcx, section_bss_begin
    mov rdi, section_bss_begin
    mov rbp, rax ; save the rax
    mov rax, 0
    rep stosb
    mov rax, rbp

    ; TODO: actually check if we have one.
    ; and maybe move this out into a CPU class in C++
    mov rdx, cr0
    and rdx, ~(EM_BIT | TS_BIT)
    mov cr0, rdx
    fninit ; initialize the FPU

    push r8
    push r9
    mov rdi, rsp

    ; Jump into kernel main
    call KERNEL_ENTRYPOINT
hang:
    cli
    hlt
    jmp hang

section .bss
align 16
kernel_stack_end:
    resb 16384
global kernel_stack_begin
kernel_stack_begin:

section .magic
    global magic_string
    ; a magic "CRC" string to verify the integrity
    magic_string: db "MAGIC_PRESENT", 0
