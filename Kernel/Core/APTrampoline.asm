BITS 16

section .text

%define addr_of(label) label - application_processor_entrypoint

ORIGIN:         equ 0x1000
ORIGIN_SEGMENT: equ (ORIGIN >> 4)

global application_processor_entrypoint:
application_processor_entrypoint:
    cli

    mov ax, ORIGIN_SEGMENT
    mov ds, ax
    mov es, ax
    mov [addr_of(alive)], byte 1

    ; wait for the bsp to acknowledge us
    wait_for_bsp:
        cmp [addr_of(allowed_to_boot)], byte 1
        jne wait_for_bsp

    ; reset the state for other APs
    mov [addr_of(alive)],           byte 0
    mov [addr_of(allowed_to_boot)], byte 0

    go_protected:
        lgdt [addr_of(gdt_entry)]

        mov eax, cr0
        or  eax, 1
        mov cr0, eax

        jmp 0x08:ORIGIN + addr_of(protected)

BITS 32
    protected:
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

    idle_loop:
        cli
        hlt
        jmp idle_loop

gdt_entry:
    dw 23 ; 3 entries * 8 bytes each - 1
    dd ORIGIN + addr_of(gdt_ptr)

gdt_ptr:
    ; NULL
    times 4 dw 0x0000

    ; 32 bit code segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x9A   ; access
    db 0xCF   ; granularity
    db 0x00   ; base

    ; 32 bit data segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x92   ; access
    db 0xCF   ; granularity
    db 0x00   ; base

global alive
alive:
    db 0

global allowed_to_boot
allowed_to_boot:
    db 0
