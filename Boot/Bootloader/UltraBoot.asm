BITS 16
ORG 0x7c00

main:
    jmp short start
    nop

%include "BPB.inc"
%include "Macros.inc"

start:
    ; initilize memory segments
    ; segment 0 offset 0x7C00
    cli
    mov [BootDrive], dl
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov si, loading_msg
    call write_string

    mov dl, [BootDrive]
    call reset_disk_system
    jc on_boot_failed

    mov si, bootloader_file
    find_file: find_file_on_disk
    mov [bootloader_cluster], ax

    fat: load_fat

    mov cx, [bootloader_cluster]
    mov ax, BOOTLOADER_SEGMENT
    mov es, ax
    read_file: read_file_from_disk

    mov ax, BOOTLOADER_SEGMENT
    mov es, ax
    mov ds, ax
    jmp BOOTLOADER_SEGMENT:0x0

%include "Utility.inc"

BOOTLOADER_SEGMENT: equ 0x1000

bootloader_cluster dw 0

bootloader_file db "UKLoaderbin"
loading_msg     db "Loading UltraOS...", CR, LF, 0x0

times 510-($-$$) db 0

dw 0xAA55
