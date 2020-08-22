BITS 16

ORG 0x7C00

main:
    jmp short start

%include "BPB.inc"
%include "CommonMacros.inc"

LOAD_KERNEL_AT:    equ 0x00100000

%ifdef ULTRA_32
KERNEL_ORIGIN:     equ 0xC0000000
KERNEL_ENTRYPOINT: equ LOAD_KERNEL_AT
%elifdef ULTRA_64
KERNEL_ORIGIN:     equ 0xFFFFFFFF80000000
PHYS_MEMORY:       equ 0xFFFF800000000000
KERNEL_ENTRYPOINT: equ KERNEL_ORIGIN + LOAD_KERNEL_AT
%endif


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
    retrieve_memory_map memory_map_ptr, memory_map.entry_count

    set_video_mode 1024, 768, 24

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
        NOTIFY_BIOS:    equ 0xEC00
        LONG_MODE_ONLY: equ 0x2

        mov ax, NOTIFY_BIOS
        mov bl, LONG_MODE_ONLY
        int 0x15
    %endif

    switch_to_protected

%ifdef ULTRA_32

jump_to_kernel:
    ; move the pointers into higher half
    add [memory_map.pointer], dword KERNEL_ORIGIN
    mov eax, context
    add eax, KERNEL_ORIGIN

    ; jump to the kernel
    jmp KERNEL_ENTRYPOINT

%elifdef ULTRA_64

PAGE_SIZE:      equ 4096
DWORDS_IN_PAGE: equ PAGE_SIZE / 4
PT_ENTRY_SIZE:  equ 8
PDT_ENTRY_SIZE: equ 8
ENTRY_COUNT:    equ 512
PAE_BIT:        equ 1 << 5
EFER_NUMBER:    equ 0xC0000080
LONG_MODE_BIT:  equ 1 << 8
PAGING_BIT:     equ 1 << 31
PRESENT:        equ 1 << 0
READWRITE:      equ 1 << 1
HUGEPAGE:       equ 1 << 7
HUGEPAGE_SIZE:  equ 0x200000

switch_to_long_mode:
    mov edi, PML4
    mov cr3, edi

    xor eax, eax
    mov ecx, DWORDS_IN_PAGE * 7 ; zero 7 pages
    rep stosd

    mov edi, cr3

    mov [edi],           dword PDPT_0 | PRESENT | READWRITE
    mov [edi + 256 * 8], dword PDPT_0 | PRESENT | READWRITE
    mov [edi + 511 * 8], dword PDPT_1 | PRESENT | READWRITE

    mov edi, dword PDT_GB0
    mov ebx, 0x00000000 | PRESENT | READWRITE | HUGEPAGE
    mov ecx, ENTRY_COUNT * 4 ; 4 tables

    set_one:
        mov [edi], ebx
        add  ebx, HUGEPAGE_SIZE
        add  edi, PDT_ENTRY_SIZE
        loop set_one

    mov  edi,  dword PDPT_0
    mov [edi + 8 * 0], dword PDT_GB0 | PRESENT | READWRITE
    mov [edi + 8 * 1], dword PDT_GB1 | PRESENT | READWRITE
    mov [edi + 8 * 2], dword PDT_GB2 | PRESENT | READWRITE
    mov [edi + 8 * 3], dword PDT_GB3 | PRESENT | READWRITE

    mov  edi,  dword PDPT_1
    mov [edi + 8 * 510], dword PDT_GB0 | PRESENT | READWRITE
    mov [edi + 8 * 511], dword PDT_GB1 | PRESENT | READWRITE

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

    ; move the pointers into higher half
    mov rax,  PHYS_MEMORY
    add rax,  [video_mode.framebuffer]
    mov qword [video_mode.framebuffer], rax

    mov rax,  PHYS_MEMORY
    add rax,  [memory_map.pointer]
    mov qword [memory_map.pointer], rax

    mov r8, PHYS_MEMORY
    add r8, context

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

PML4:    equ 0x10000
PDPT_0:  equ 0x11000 ; identity 4 GB
PDPT_1:  equ 0x12000 ; kernel 2 GB

PDT_GB0: equ 0x13000
PDT_GB1: equ 0x14000
PDT_GB2: equ 0x15000
PDT_GB3: equ 0x16000

%endif

kernel_file db "Kernel  bin"

kernel_sector: times SECTOR_SIZE db 0
gdt_ptr:
    .null: equ $ - gdt_ptr
    dq 0x0000000000000000

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

best_mode:     dw 0
native_width:  dw 1024
native_height: dw 768

edid:
    times 0x38 db 0 ; unused
    .horizontal_active_lower:  db 0
    db 0 ; unused
    .horizontal_active_higher: db 0
    .vertical_active_lower:    db 0
    db 0 ; unused
    .vertical_active_higher: db 0

    times 128 - ($ - edid) db 0 ; unused

vbe_info:
    .signature:     db "VBE2"
    .version:       dw 0
    .oem:           dd 0
    .capabilities:  dd 0
    .modes_offset:  dw 0
    .modes_segment: dw 0
    .video_memory:  dw 0
    .software_rev:  dw 0
    .vendor:        dd 0
    .product_name:  dd 0
    .product_rev:   dd 0
    .unused: times 222 + 256 db 0

vbe_mode_info:
    .attributes:   dw 0
    .window_a:     db 0
    .window_b:     db 0
    .granularity:  dw 0
    .window_size:  dw 0
    .segment_a:    dw 0
    .segment_b:    dw 0
    .win_func_ptr: dd 0
    .pitch:        dw 0
    .width:        dw 0
    .height:       dw 0

    .w_char:       db 0
    .y_char:       db 0
    .planes:       db 0
    .bpp:          db 0
    .banks:        db 0
    .memory_model: db 0
    .bank_size:    db 0
    .image_pages:  db 0
    .reserved_0:   db 0

    .red_mask:                db 0
    .red_position:            db 0
    .green_mask:              db 0
    .green_position:          db 0
    .blue_mask:               db 0
    .blue_position:           db 0
    .reserved_mask:           db 0
    .reserved_position:       db 0
    .direct_color_attributes: db 0

    .framebuffer:          dd 0
    .off_screen_mem_off:   dd 0
    .off_screen_mem_size:  dw 0
    .reserved_1: times 206 db 0

context:
    video_mode:
        .width:  dd 0
        .height: dd 0
        .pitch:  dd 0
        .bpp:    dd 0

        %ifdef ULTRA_32
        .framebuffer: dd 0
        %elifdef ULTRA_64
        .framebuffer: dq 0
        %endif
    memory_map:
        %ifdef ULTRA_32
        .pointer: dd memory_map_ptr
        %elifdef ULTRA_64
        .pointer: dq memory_map_ptr
        %endif
        .entry_count: dd 0

memory_map_ptr:
