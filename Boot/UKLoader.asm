BITS 16

ORG 0x7C00

main:
    jmp short start

%include "BPB.inc"
%include "CommonMacros.inc"

KERNEL_SEGMENT:         equ 0x2000
KERNEL_FLAT_ADDRESS:    equ (KERNEL_SEGMENT << 4)
KERNEL_DIRECTORY_INDEX: equ 1

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

    ; read the first sector of the root directory
    read_root_directory [boot_drive], KERNEL_SEGMENT, 0x0000

    read_directory_file_extended KERNEL_SEGMENT, KERNEL_DIRECTORY_INDEX, kernel_file

    mov si, loading_msg
    call write_string

    ; enable color 80x25 text mode
    ; for future use in protected mode
    mov ax, 0x0003
    int 0x10

    ; disable cursor
    mov ax, 0x0100
    mov cx, 0x3F00
    int 0x10

    ; ---- set up IDT and GDT ----
    xor ax, ax
    mov es, ax
    mov di, ax
    mov cx, 2048
    rep stosb ; 256 empty IDT entries

    ; NULL descriptor
    mov cx, 4
    rep stosw

    ; code segment descriptor
    mov [es:di],     word 0xFFFF ; limit
    mov [es:di + 2], word 0x0000 ; base
    mov [es:di + 4], byte 0x00   ; base
    mov [es:di + 5], byte 0x9A   ; access
    mov [es:di + 6], byte 0xCF   ; granularity
    mov [es:di + 7], byte 0x00   ; base
    add di, 8

    ; data segment descriptor
    mov [es:di],     word 0xFFFF ; limit
    mov [es:di + 2], word 0x0000 ; base
    mov [es:di + 4], byte 0x00   ; base
    mov [es:di + 5], byte 0x92   ; access
    mov [es:di + 6], byte 0xCF   ; granularity
    mov [es:di + 7], byte 0x00   ; base

    cli

    lgdt [gdt_entry]
    lidt [idt_entry]

    ; switch to protected mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    jmp 0x08:jump_to_kernel

BITS 32
jump_to_kernel:
    ; setup segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x30000

    ; jump to the kernel
    jmp KERNEL_FLAT_ADDRESS
BITS 16

%include "Common.inc"
%include "LoaderUtils.inc"

idt_entry:
    dw 2048
    dd 0

gdt_entry:
    dw 24
    dd 2048

boot_context:
    this_partition:         dw 0x0000
    data_offset_in_sectors: dd 0x00000000
    fat_offset_in_sectors:  dd 0x00000000

dskread_msg:        db "Reading kernel from disk...", CR, LF, 0
loading_msg:        db "Preparing kernel environment...", CR, LF, 0
a20fail_msg:        db "Failed to enable A20!", CR, LF, 0
no_file_error:      db "Couldn't find the kernel file!", CR, LF, 0

kernel_file db "Kernel  bin"
