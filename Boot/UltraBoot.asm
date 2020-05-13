BITS 16
ORG 0x7c00

KERNEL_LOADER_SEGMENT: equ 0x1000

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

    ; calculate offset to FAT
    movzx eax, word [sectors_reserved]
    add   eax, [hidden_sector_count]
    mov [fat_offset_in_sectors], eax

    ; calculate the offset to data in sectors
    mov   eax, [sectors_per_fat_table]
    movzx ebx, byte [fat_table_count]
    mul   ebx
    add   eax, [fat_offset_in_sectors]

    mov [data_offset_in_sectors], eax

    read_root_directory [boot_drive], KERNEL_LOADER_SEGMENT

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
        mov bx, [bytes_per_sector]
        div ebx
        cmp edx, 0
        je aligned ; fix this hack :D
        add eax, 1
        aligned:
        mov [DAP.sector_count], ax

        movzx eax, word [es:CLUSTER_LOW_ENTRY_OFFSET]
        sub   eax, RESERVED_CLUSTER_COUNT
        movzx edx, byte [sectors_per_cluster]
        mul edx
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
        mov si, boot_context
        jmp KERNEL_LOADER_SEGMENT:0x0

    on_file_not_found:
        mov si, no_file_error
        call write_string
        call reboot

%include "Common.inc"

kernel_loader_file:     db "UKLoaderbin"
no_file_error:          db "Couldn't find the kernel loader file!", CR, LF, 0

boot_context:
    this_partition:         dw 0x0000
    data_offset_in_sectors: dd 0x00000000
    fat_offset_in_sectors:  dd 0x00000000

times 510-($-$$) db 0
boot_signature   dw 0xAA55