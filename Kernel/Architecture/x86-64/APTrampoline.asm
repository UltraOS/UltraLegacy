BITS 16

%define ap_entrypoint _ZN6kernel3CPU13ap_entrypointEv
extern  ap_entrypoint

section .text

VIRTUAL_ORIGIN:  equ 0xFFFFFFFF80000000
ORIGIN:          equ 0x1000
TEMPO_STACK:     equ 0x7000
PRESENT:         equ 0b01
READWRITE:       equ 0b10
PAE_BIT:         equ 1 << 5
EFER_NUMBER:     equ 0xC0000080
LONG_MODE_BIT:   equ 1 << 8
PAGING_BIT:      equ 1 << 31
PROTECTED_BIT:   equ 1 << 0

%define TRUE  byte 1
%define FALSE byte 0

ALIVE:        equ 0x500 ; 1 byte bool
ACKNOWLEDGED: equ 0x510 ; 1 byte bool
STACK:        equ 0x520 ; 8 byte address
PML4:         equ 0x530 ; 4 byte address

%define ADDR_OF(label) ORIGIN + label - application_processor_entrypoint
%define TO_PHYSICAL(virtual) (virtual - VIRTUAL_ORIGIN)
%define TO_VIRTUAL(physical) (physical + VIRTUAL_ORIGIN)

global application_processor_entrypoint:
application_processor_entrypoint:
    cli

    mov ax, 0x0
    mov ds, ax
    mov es, ax
    mov [ALIVE], TRUE

    ; wait for the bsp to acknowledge us
    wait_for_bsp:
        cmp [ACKNOWLEDGED], TRUE
        jne wait_for_bsp

    ; reset the state for other APs
    mov [ALIVE],        FALSE
    mov [ACKNOWLEDGED], FALSE

    ; Identity map bottom
    mov eax, [PML4]
    mov cr3, eax

    mov eax, cr4
    or  eax, PAE_BIT
    mov cr4, eax

    mov ecx, EFER_NUMBER
    rdmsr
    or eax, LONG_MODE_BIT
    wrmsr

    mov eax, cr0
    or  eax, PAGING_BIT | PROTECTED_BIT
    mov cr0, eax

    lgdt [ADDR_OF(gdt_entry)]

    jmp gdt_ptr.code_64:ADDR_OF(long_mode)

BITS 64

extern ap_entrypoint

    long_mode:
        mov rax, higher_half
        jmp rax

    higher_half:
        mov rsp, [STACK]

        mov rax, cr3
        mov  qword [rax], 0x0000000000000000
        mov cr3, rax

        mov  rax, ap_entrypoint
        call rax

    idle_loop:
        cli
        hlt
        jmp idle_loop

gdt_entry:
    dw gdt_end - gdt_ptr - 1
    dd ADDR_OF(gdt_ptr)

gdt_ptr:
    .null: equ $ - gdt_ptr
    dq 0x0000000000000000

    .data_32: equ $ - gdt_ptr
    ; 32 bit data segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x92   ; access
    db 0xCF   ; granularity
    db 0x00   ; base

    .code_64: equ $ - gdt_ptr
    ; 64 bit code segment
    dw 0
    dw 0
    db 0
    db 0x9A
    db 0xAF
    db 0
gdt_end:
