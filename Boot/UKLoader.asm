BITS 16

ORG 0x7C00

main:
    jmp short start

%include "BPB.inc"
%include "CommonMacros.inc"

SIZEOF_IDT:        equ 2048
SIZEOF_GDT:        equ 40
LOAD_KERNEL_AT:    equ 0x00100000
KERNEL_ENTRYPOINT: equ 0xFFFFFFFF80000000 + LOAD_KERNEL_AT

start:
    ; copy the boot context from VBR
    push ds
    xor ax, ax
    mov ds, ax
    mov di, boot_context
    mov cx, BOOT_CONTEXT_SIZE
    rep movsb

    ; copy the BPB from VBR
    xor ax, ax
    mov ds, ax
    mov si, VBR_ORIGIN + BPB_OFFSET
    mov di, BPB
    mov cx, BPB_SIZE
    rep movsb
    pop ds

    ; ---- try enable the A20 line ----
    enable_a20:
        ; check if its already enabled
        call is_a20_enabled
        cmp ax, 1
        je a20_success

        ; try using the BIOS interrupt 15
        call try_enable_a20_using_bios
        call is_a20_enabled
        cmp ax, 1
        je a20_success

        ; try using the 8042 controller
        call try_enable_a20_using_8042
        call is_a20_enabled
        cmp ax, 1
        je a20_success

        ; try using fast gate
        call try_enable_a20_using_fast_gate
        call is_a20_enabled
        cmp ax, 1
        je a20_success

    a20_failure:
        mov si, a20fail_msg
        call write_string
        call reboot

    a20_success:

    mov si, dskread_msg
    call write_string

    mov si, loading_msg
    call write_string

    ; read the first sector of the root directory
    read_root_directory [boot_drive], FAT_DIRECTORY_SEGMENT, 0x0000

    read_directory_file_protected LOAD_KERNEL_AT, 1, kernel_file

    xor ax, ax
    mov es, ax
    retrieve_memory_map memory_map, memory_map_entry_count

    %ifdef ULTRA_64

    GET_HIGHEST_EXTENDED_FUNCTION: equ 0x80000000
    EXTENDED_PROCESSOR_INFO:       equ 0x80000001
    LONG_MODE_SUPPORTED_BIT:       equ 1 << 29

    ; TODO: we kinda assmume that CPUID is supported here, should we check for it?

    mov eax, GET_HIGHEST_EXTENDED_FUNCTION
    cpuid
    cmp eax, EXTENDED_PROCESSOR_INFO
    jl no_long_mode

    mov eax, EXTENDED_PROCESSOR_INFO
    cpuid
    test edx, LONG_MODE_SUPPORTED_BIT
    jnz long_mode_supported

    no_long_mode:
        mov si, no_long_mode_message
        call write_string
        call reboot

    long_mode_supported:
        NOTIFY_BIOS:    0xEC00
        LONG_MODE_ONLY: 0x2

        mov ax, NOTIFY_BIOS
        mov bl, LONG_MODE_ONLY
        int 0x15
    %endif

    ; enable color 80x25 text mode
    ; for future use in protected/long mode
    mov ax, 0x0003
    int 0x10

    ; disable cursor
    mov ax, 0x0100
    mov cx, 0x3F00
    int 0x10

    switch_to_protected

%ifdef ULTRA_32

jump_to_kernel:
    ; pass kernel the memory map
    mov   eax, memory_map
    movzx ebx, word [memory_map_entry_count]

    ; jump to the kernel
    jmp LOAD_KERNEL_AT

%elifdef ULTRA_64

PAGE_SIZE:      equ 4096
DWORDS_IN_PAGE: equ PAGE_SIZE / 4
PT_ENTRY_SIZE:  equ 8
ENTRY_COUNT:    equ 512
PAE_BIT:        equ 1 << 5
EFER_NUMBER:    equ 0xC0000080
LONG_MODE_BIT:  equ 1 << 8
PAGING_BIT:     equ 1 << 31
PRESENT:        equ 1 << 0
READWRITE:      equ 1 << 1

switch_to_long_mode:
    mov edi, PML4
    mov cr3, edi

    xor eax, eax
    mov ecx, DWORDS_IN_PAGE * 10 ; zero 10 pages
    rep stosd

    mov edi, cr3

    mov [edi], dword PDPT_0 | PRESENT | READWRITE
    add  edi,  PAGE_SIZE
    mov [edi], dword PDT | PRESENT | READWRITE
    add  edi,  PAGE_SIZE * 3
    mov [edi     ], dword PT_0 | PRESENT | READWRITE
    mov [edi + 8 ], dword PT_1 | PRESENT | READWRITE
    mov [edi + 16], dword PT_2 | PRESENT | READWRITE
    mov [edi + 24], dword PT_3 | PRESENT | READWRITE
    mov [edi + 32], dword PT_4 | PRESENT | READWRITE
    add  edi,  PAGE_SIZE

    mov ebx, 0x00000000   | PRESENT | READWRITE
    mov ecx, ENTRY_COUNT * 5 ; 5 page tables

    set_one:
        mov [edi], ebx
        add  ebx, PAGE_SIZE
        add  edi, PT_ENTRY_SIZE
        loop set_one

    mov  edi,  PDPT_3
    mov [edi], dword PDT | PRESENT | READWRITE

    mov edi, PML4 + 256 * 8
    mov [edi], dword PDPT_3 | PRESENT | READWRITE

    mov  edi, PDPT_1 + 510 * 8
    mov [edi], dword PDT | PRESENT | READWRITE

    mov edi, PML4 + 511 * 8
    mov [edi], dword PDPT_1 | PRESENT | READWRITE

    mov eax, cr4
    or  eax, PAE_BIT
    mov cr4, eax

    mov ecx, EFER_NUMBER
    rdmsr
    or eax, LONG_MODE_BIT
    wrmsr

    mov eax, cr0
    or  eax, PAGING_BIT
    mov cr0, eax

    lgdt [gdt_entry]

    jmp gdt_ptr.code_64:jump_to_kernel

BITS 64
jump_to_kernel:
    mov ax, gdt_ptr.data_32
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    cli
    hlt

    jmp KERNEL_ENTRYPOINT

%else
    %error Couldn't detect the target architecture
%endif

BITS 16

%include "Common.inc"
%include "LoaderUtils.inc"

sectors_to_read: dd 0
sector_offset:   dd 0
memory_offset:   dd 0

gdt_entry:
    dw gdt_end - gdt_ptr - 1
    dd gdt_ptr

boot_context:
    this_partition:         dw 0x0000
    data_offset_in_sectors: dd 0x00000000
    fat_offset_in_sectors:  dd 0x00000000

dskread_msg:   db "Reading kernel from disk...", CR, LF, 0
loading_msg:   db "Preparing kernel environment...", CR, LF, 0
a20fail_msg:   db "Failed to enable A20!", CR, LF, 0
no_file_error: db "Couldn't find the kernel file!", CR, LF, 0

%ifdef ULTRA_64
no_long_mode_message: db "This CPU doesn't support x86_64", CR, LF, 0

PML4:   equ 0x10000
PDPT_0: equ 0x11000
PDPT_1: equ 0x12000
PDPT_3: equ 0x13000
PDT:    equ 0x14000

; 10 MB
PT_0: equ 0x15000
PT_1: equ 0x16000
PT_2: equ 0x17000
PT_3: equ 0x18000
PT_4: equ 0x19000

%endif

kernel_file db "Kernel  bin"

kernel_sector: times SECTOR_SIZE db 0
gdt_ptr:
    .null: equ $ - gdt_ptr
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x98   ; access
    db 0x00   ; granularity
    db 0x00   ; base

    .code_16: equ $ - gdt_ptr
    ; 16 bit code segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x98   ; access
    db 0x00   ; granularity
    db 0x00   ; base

    .data_16: equ $ - gdt_ptr
    ; 16 bit data segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x92   ; access
    db 0x00   ; granularity
    db 0x00   ; base

    .code_32: equ $ - gdt_ptr
    ; 32 bit code segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x9A   ; access
    db 0xCF   ; granularity
    db 0x00   ; base

    .data_32: equ $ - gdt_ptr
    ; 32 bit data segment descriptor
    dw 0xFFFF ; limit
    dw 0x0000 ; base
    db 0x00   ; base
    db 0x92   ; access
    db 0xCF   ; granularity
    db 0x00   ; base

    %ifdef ULTRA_64
    .code_64: equ $ - gdt_ptr
    ; 64 bit code segment
    dw 0
    dw 0
    db 0
    db 0x9A
    db 0xAF
    db 0

    %endif
gdt_end:

memory_map_entry_count: dw 0x0000
memory_map:
