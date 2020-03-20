BITS 16
ORG 0x7c00

CR: equ 0x0D
LF: equ 0x0A

STAGE_TWO_LOAD_SEGMENT: equ 0x1000


main:
    jmp short start
    nop

boot_sector:
    ; BIOS Parameter Block
    OEMLabel          db "UltraOS "
    BytesPerSector    dw 512
    SectorsPerCluster db 1
    SectorsReserved   dw 1
    FATCount          db 2
    RootDirEntries    dw 224
    SectorsCount      dw 2880
    MediaDescriptor   db 0xF0 ; gonna pretend we're a 3.5 floppy
    FATSize           dw 9
    SectorsPerTrack   dw 9
    NumberOfSides     dw 2
    SectorsHidden     dd 0
    SectorsLarge      dd 0

    ; Extended BIOS Parameter Block
    BootDrive         db 0
    Reserved          db 0
    BootSignature     db 0x29
    VolumeID          dd 0
    VolumeLabel       db "UltraVolume"
    FileSystem        db "FAT16   "

write_string:
    lodsb
    or al, al
    jz .done

    mov ah, 0xE
    mov bx, 9
    int 0x10

    jmp write_string
.done:
    ret

on_boot_failed:
    mov si, disk_error_msg
    call write_string
    call reboot

reboot:
    mov si, reboot_msg
    call write_string
    xor ax, ax
    int 0x16
    jmp word 0xFFFF:0x0000

start:
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
    xor ax, ax
    int 0x13
    jc on_boot_failed

    call reboot

loading_msg    db "Loading UltraOS...", CR, LF, 0x0
disk_error_msg db "Error loading disk.", CR, LF, 0x0
reboot_msg     db "Press any key to reboot.", CR, LF, 0x0

times 510-($-$$) db 0

dw 0xAA55
