BITS 16
ORG 0x7c00

main:
    jmp short start
    nop

%include "BPB.inc"
%include "CommonMacros.inc"

start:
    ; initilize memory segments:
    ; segment 0 
    ; offset  0x7C00
    ; stack   0x7C00
    cli
    mov [BootDrive], dl
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    clear_screen

    mov si, loading_msg
    call write_string

    mov dl, [BootDrive]
    call reset_disk_system
    jc on_boot_failed

    ; read the kernel loader from disk
    find_file_on_disk kernel_loader_file, KERNEL_LOADER_SEGMENT
    mov [kernel_loader_cluster], ax

    load_fat

    read_file_from_disk [kernel_loader_cluster], KERNEL_LOADER_SEGMENT

    ; jump to kernel loader
    ; it expects the segments to be initilized
    ; according to its origin so we set them here
    mov ax, KERNEL_LOADER_SEGMENT
    mov es, ax
    mov ds, ax
    jmp KERNEL_LOADER_SEGMENT:0x0

%include "Common.inc"

KERNEL_LOADER_SEGMENT: equ 0x1000

kernel_loader_cluster dw 0
kernel_loader_file    db "UKLoaderbin"

loading_msg db "Loading UltraOS...", CR, LF, 0x0

times 510-($-$$) db 0
boot_signature   dw 0xAA55
