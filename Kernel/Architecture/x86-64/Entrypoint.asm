%define KERNEL_ENTRYPOINT _ZN6kernel3runEPNS_7ContextE

extern KERNEL_ENTRYPOINT

section .entry

extern section_bss_begin
extern section_bss_end

EM_BIT: equ (1 << 2)
TS_BIT: equ (1 << 3)
VIRTUAL_ORIGIN: equ 0xFFFFFFFF80000000

global start
start:
    mov rax, cr3
    mov qword [rax], 0x0000000000000000
    mov cr3, rax

    ; Set up the kernel stack
    mov rsp, kernel_stack_begin

    ; zero section bss
    mov rcx, section_bss_end
    sub rcx, section_bss_begin
    mov rdi, section_bss_begin
    mov rax, 0
    rep stosb

    ; TODO: actually check if we have one.
    ; and maybe move this out into a CPU class in C++
    mov rdx, cr0
    and rdx, ~(EM_BIT | TS_BIT)
    mov cr0, rdx
    fninit ; initialize the FPU

    ; boot context
    mov rdi, r8

    xor rbp, rbp
    ; Jump into kernel main
    call KERNEL_ENTRYPOINT
hang:
    cli
    hlt
    jmp hang

section .bss
align 16
kernel_stack_end:
    resb 32768
global kernel_stack_begin
kernel_stack_begin:

section .magic
    global magic_string
    ; a magic "CRC" string to verify the integrity
    magic_string: db "MAGIC_PRESENT", 0
