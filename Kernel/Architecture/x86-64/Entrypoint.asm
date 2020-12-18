%define KERNEL_ENTRYPOINT _ZN6kernel3runEPNS_13LoaderContextE

extern KERNEL_ENTRYPOINT

extern section_bss_begin
extern section_bss_end

EM_BIT: equ (1 << 2)
TS_BIT: equ (1 << 3)
VIRTUAL_ORIGIN: equ 0xFFFFFFFF80000000
PRESENT:        equ (1 << 0)
READWRITE:      equ (1 << 1)
HUGEPAGE:       equ (1 << 7)
ENTRY_COUNT:    equ 512
HUGEPAGE_SIZE:  equ 0x200000
PAGE_SIZE:      equ 4096
PDT_ENTRY_SIZE: equ 8

%define TO_PHYSICAL(virtual) (virtual - VIRTUAL_ORIGIN)

section .bss
; can't use PAGE_SIZE here because of a NASM bug
; it simply ignores this if it's a macro
align 4096

; PML4 in this case
global kernel_base_table
kernel_base_table:
    resb PAGE_SIZE

direct_map_pdpt:
    resb PAGE_SIZE

direct_map_pdt_gb0:
    resb PAGE_SIZE

direct_map_pdt_gb1:
    resb PAGE_SIZE

direct_map_pdt_gb2:
    resb PAGE_SIZE

direct_map_pdt_gb3:
    resb PAGE_SIZE

kernel_pdpt:
    resb PAGE_SIZE

global kernel_pdt
kernel_pdt:
    resb PAGE_SIZE

global memory_map_entries_buffer
memory_map_entries_buffer:
    resb PAGE_SIZE

section .entry

global start
start:
    ; Set up the kernel stack
    mov rsp, kernel_stack_begin

    ; zero section bss
    mov rcx, section_bss_end
    sub rcx, section_bss_begin
    mov rdi, section_bss_begin
    mov rax, 0
    rep stosb

    ; set PML4 up with direct map & kernel PDPTs
    mov rdi, kernel_base_table

    mov rax, TO_PHYSICAL(direct_map_pdpt) + (PRESENT | READWRITE)
    mov [rdi + 256 * 8], rax

    mov rax, TO_PHYSICAL(kernel_pdpt) + (PRESENT | READWRITE)
    mov [rdi + 511 * 8], rax
    
    mov rdi, direct_map_pdt_gb0
    mov rbx, 0x0000000000000000 + (PRESENT | READWRITE | HUGEPAGE)
    mov rcx, ENTRY_COUNT * 4 ; 4 tables
    
    set_one:
        mov [rdi], rbx
        add  rbx, HUGEPAGE_SIZE
        add  rdi, PDT_ENTRY_SIZE
        loop set_one

    ; direct map first 4GB of physical memory at PHYSICAL_BASE
    mov rdi, direct_map_pdpt

    mov rax, TO_PHYSICAL(direct_map_pdt_gb0) + (PRESENT | READWRITE)
    mov [rdi + 8 * 0], rax

    mov rax, TO_PHYSICAL(direct_map_pdt_gb1) + (PRESENT | READWRITE)
    mov [rdi + 8 * 1], rax

    mov rax, TO_PHYSICAL(direct_map_pdt_gb2) + (PRESENT | READWRITE)
    mov [rdi + 8 * 2], rax

    mov rax, TO_PHYSICAL(direct_map_pdt_gb3) + (PRESENT | READWRITE)
    mov [rdi + 8 * 3], rax

    ; put the kernel direct mapping PDT at -2GB
    mov rdi, kernel_pdpt
    mov rax, TO_PHYSICAL(kernel_pdt) + (PRESENT | READWRITE)
    mov [rdi + 8 * 510], rax

    ; direct map the first 4 megabytes for the kernel image
    mov rdi, kernel_pdt
    mov rax, 0x0000000000000000 + (PRESENT | READWRITE | HUGEPAGE)
    mov [rdi + 8 * 0], rax
    add rax, HUGEPAGE_SIZE
    mov [rdi + 8 * 1], rax

    mov rax, TO_PHYSICAL(kernel_base_table)
    mov cr3, rax

    ; initialize the FPU
    ; TODO: actually check if we have one, and maybe move this out into a CPU class in C++.
    mov rdx, cr0
    and rdx, ~(EM_BIT | TS_BIT)
    mov cr0, rdx
    fninit

    ; zero rbp to help backtracer identify the first frame
    xor rbp, rbp

    ; boot context pointer as arg1
    mov rdi, r8

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
