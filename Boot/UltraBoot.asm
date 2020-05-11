BITS 16
ORG 0x7c00

main:
    jmp short start
    nop

OEMLabel db "UltraOS "
%include "BPB.inc"
%include "CommonMacros.inc"

start:
    ; initilize memory segments
    cli
    mov [boot_drive], dl
    mov [this_partition], bx
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; calculate the offset to data in sectors
    mov eax, [SectorsPerFAT]
    xor ebx, ebx
    mov bx,  [FATCount]
    mul ebx
    add ax,  [SectorsReserved]
    add eax, [SectorsHidden]
    mov [data_offset_in_sectors], eax

    ; calculate offset to FAT
    mov eax, [SectorsHidden]
    add ax,  [SectorsReserved]
    mov [fat_offset_in_sectors], eax

    ; read the first sector of the root directory
    mov dl, [boot_drive]
    mov ax, KERNEL_LOADER_SEGMENT
    mov es, ax
    xor di, di
    mov eax, [RootDirCluster]
    sub eax, RESERVED_CLUSTER_COUNT
    add eax, [data_offset_in_sectors]
    call read_disk_lba

    ; check if this file is what we're looking for
    mov cx, FAT_FILENAME_LENGTH
    mov ax, KERNEL_LOADER_SEGMENT
    mov es, ax
    xor di, di
    mov si, kernel_loader_file
    repz cmpsb
    jne on_file_not_found

    on_file_found:
        mov eax, [es:FILESIZE_ENTRY_OFFSET]
        xor ebx, ebx
        xor edx, edx
        mov bx, [BytesPerSector]
        div ebx
        cmp edx, 0
        je aligned ; fix this hack :D
        add eax, 1
        aligned:
        mov [DAP.sector_count], ax

        mov ax, [es:CLUSTER_LOW_ENTRY_OFFSET]
        sub eax, RESERVED_CLUSTER_COUNT
        add eax, [data_offset_in_sectors]

        push eax
        mov ax, KERNEL_LOADER_SEGMENT
        mov es, ax
        xor di, di
        pop eax
        mov dl, [boot_drive]

        call read_disk_lba

    transfer_control_to_kernel_loader:
        mov ax, KERNEL_LOADER_SEGMENT
        mov ds, ax
        mov es, ax
        mov ax, boot_context
        jmp KERNEL_LOADER_SEGMENT:0x0

    on_file_not_found:
        mov si, no_file_error
        call write_string
        call reboot

%include "Common.inc"

CLUSTER_HIGH_ENTRY_OFFSET: equ 20
CLUSTER_LOW_ENTRY_OFFSET:  equ 26
FILESIZE_ENTRY_OFFSET:     equ 28
RESERVED_CLUSTER_COUNT:    equ 2
KERNEL_LOADER_SEGMENT:     equ 0x1000
FAT_SEGMENT:               equ 0x2000
FAT_FILENAME_LENGTH:       equ 11
ENTRIES_PER_FAT_SECTOR:    equ 128
END_OF_CHAIN:              equ 0x0FFFFFF8
KERNEL_LOADER_CLUSTER:     equ 3

kernel_loader_file:     db "UKLoaderbin"
no_file_error:          db "Couldn't find the kernel loader file!", CR, LF, 0

boot_context:
    boot_drive:             db 0
    this_partition:         dw 0x0
    data_offset_in_sectors: dd 0x0
    fat_offset_in_sectors:  dd 0x0

times 510-($-$$) db 0
boot_signature   dw 0xAA55