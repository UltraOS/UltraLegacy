%ifndef _COMMON_MACROS_INC_
%define _COMMON_MACROS_INC_

CR: equ 0x0D
LF: equ 0x0A

BOOT_CONTEXT_SIZE:         equ 10
CLUSTER_LOW_ENTRY_OFFSET:  equ 26
FILESIZE_ENTRY_OFFSET:     equ 28
RESERVED_CLUSTER_COUNT:    equ 2
FAT_FILENAME_LENGTH:       equ 11
VBR_ORIGIN:                equ 0x7C00
BPB_OFFSET:                equ 11
BPB_SIZE:                  equ 79 ; FAT32
DIRECTORY_ENTRY_SIZE:      equ 32

; magic breakpoint in Bochs
%define BP xchg bx, bx

; void read_root_directory(boot_dirve, into_segment)
%macro read_root_directory 2
%%read_root_dir:
    ; read the first sector of the root directory
    mov dl, %1
    mov ax, %2
    mov es, ax
    xor di, di
    mov eax, [root_directory_cluster]
    sub eax, RESERVED_CLUSTER_COUNT
    add eax, [data_offset_in_sectors]
    call read_disk_lba
%endmacro

; void read_directory_file(dir_segment, index, filename)
%macro read_directory_file 3
%%read_directory_file:
    ; check if this file is what we're looking for
    mov cx, FAT_FILENAME_LENGTH
    mov ax, %1
    mov es, ax
    mov di, %2 * DIRECTORY_ENTRY_SIZE
    mov si, %3
    repz cmpsb
    je %%.on_file_found

    %%.on_file_not_found:
        mov si, no_file_error
        call write_string
        call reboot

    %%.on_file_found:
        mov eax, [es:%2 * DIRECTORY_ENTRY_SIZE + FILESIZE_ENTRY_OFFSET]
        xor ebx, ebx
        xor edx, edx
        mov bx, [bytes_per_sector]
        div ebx
        test edx, edx
        jz aligned
        inc eax
        aligned:
        mov [DAP.sector_count], ax

        movzx eax, word [es:%2 * DIRECTORY_ENTRY_SIZE + CLUSTER_LOW_ENTRY_OFFSET]
        sub   eax, RESERVED_CLUSTER_COUNT
        movzx edx, byte [sectors_per_cluster]
        mul edx
        add eax, [data_offset_in_sectors]

        push eax
        mov ax, %1
        mov es, ax
        xor di, di
        pop eax
        mov dl, [boot_drive]

        call read_disk_lba
%endmacro

%endif ; _COMMON_MACROS_INC_
