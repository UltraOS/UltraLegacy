BITS 16

%define ap_entrypoint _ZN6kernel3CPU13ap_entrypointEv
extern  ap_entrypoint

section .text

VIRTUAL_ORIGIN:  equ 0xC0000000
ORIGIN:          equ 0x1000
TEMPO_STACK:     equ 0x7000
PRESENT:         equ 0b01
READWRITE:       equ 0b10
WRITE_PROTECT:   equ (0b1 << 16)
PAGING:          equ (0b1 << 31)

%define TRUE  byte 1
%define FALSE byte 0

ALIVE:        equ 0x500 ; 1 byte bool
ACKNOWLEDGED: equ 0x510 ; 1 byte bool
STACK:        equ 0x520 ; 4 byte address

%define ADDR_OF(label) ORIGIN + label - application_processor_entrypoint
%define TO_PHYSICAL(virtual) (virtual - VIRTUAL_ORIGIN)

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

    go_protected:
        lgdt [ADDR_OF(gdt_entry)]

        mov eax, cr0
        or  eax, 1
        mov cr0, eax

        jmp gdt_ptr.code:ADDR_OF(protected)

BITS 32

extern kernel_page_directory
extern kernel_page_table

    protected:
        mov ax, gdt_ptr.data
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

    paging:
        ; identity mapping to enable paging
        mov [TO_PHYSICAL(kernel_page_directory)], dword (TO_PHYSICAL(kernel_page_table) + (PRESENT | READWRITE))

        mov ecx, TO_PHYSICAL(kernel_page_directory)
        mov cr3, ecx

        mov ecx, cr0
        or  ecx, (WRITE_PROTECT | PAGING)
        mov cr0, ecx

        mov ecx, ADDR_OF(higher_half) + VIRTUAL_ORIGIN
        jmp ecx

    higher_half:
        ; Set up the kernel stack
        mov esp, [STACK]

        ; remove the identity mapping here
        mov [kernel_page_directory], dword 0x00000000

        ; flush TLB
        mov ecx, cr3
        mov cr3, ecx
        invlpg [0x00100000]

        ; Jump into kernel main
        mov eax, ap_entrypoint
        call eax

    idle_loop:
        cli
        hlt
        jmp idle_loop

gdt_entry:
    dw gdt_end - gdt_ptr - 1
    dd ADDR_OF(gdt_ptr)

gdt_ptr:
    ; NULL
    .null: equ $ - gdt_ptr
    dq 0x00000000

    .code: equ $ - gdt_ptr
    ; 32 bit code segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x9A   ; access
    db 0xCF   ; granularity
    db 0x00   ; base

    .data: equ $ - gdt_ptr
    ; 32 bit data segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x92   ; access
    db 0xCF   ; granularity
    db 0x00   ; base
gdt_end:
