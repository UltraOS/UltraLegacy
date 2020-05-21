%ifndef _COMMON_MACROS_INC_
%define _COMMON_MACROS_INC_

CR: equ 0x0D
LF: equ 0x0A

BOOT_CONTEXT_SIZE:         equ 10
CLUSTER_LOW_ENTRY_OFFSET:  equ 26
FILESIZE_ENTRY_OFFSET:     equ 28
RESERVED_CLUSTER_COUNT:    equ 2
FAT_FILENAME_LENGTH:       equ 11
VBR_ORIGIN:                equ 0xFDFF
KERNEL_LOADER_SEGMENT:     equ 0x0000
KERNEL_LOADER_OFFSET:      equ 0x7C00
BPB_OFFSET:                equ 11
BPB_SIZE:                  equ 79 ; FAT32
DIRECTORY_ENTRY_SIZE:      equ 32
MAX_SECTOR_COUNT_PER_READ: equ 127
SECTOR_PER_SEGMENT_COUNT:  equ 128
SECTOR_SIZE:               equ 512 ; also present in BPB but this one is compile time
SECTOR_OVERFLOW_COUNT:     equ SECTOR_PER_SEGMENT_COUNT - MAX_SECTOR_COUNT_PER_READ
MAX_BYTE_COUNT_PER_READ:   equ MAX_SECTOR_COUNT_PER_READ * SECTOR_SIZE

; magic breakpoint in Bochs
%define BP xchg bx, bx

; void read_root_directory(boot_dirve, into_segment, into_offset)
%macro read_root_directory 3
%%read_root_dir:
    ; read the first sector of the root directory
    mov dl, %1
    mov ax, %2
    mov es, ax
    mov ax, %3
    mov di, ax
    mov eax, [root_directory_cluster]
    sub eax, RESERVED_CLUSTER_COUNT
    add eax, [data_offset_in_sectors]
    call read_disk_lba
%endmacro

; void read_directory_file(dir_segment, dir_offset, index, filename)
%macro read_directory_file 4
%%read_directory_file:
    ; check if this file is what we're looking for
    mov cx, FAT_FILENAME_LENGTH
    mov ax, %1
    mov es, ax
    mov di, %2 + %3 * DIRECTORY_ENTRY_SIZE
    mov si, %4
    repz cmpsb
    je %%.on_file_found

    %%.on_file_not_found:
        mov si, no_file_error
        call write_string
        call reboot

    %%.on_file_found:
        mov eax, [es:%2 + %3 * DIRECTORY_ENTRY_SIZE + FILESIZE_ENTRY_OFFSET]
        xor ebx, ebx
        xor edx, edx
        mov bx, [bytes_per_sector]
        div ebx
        test edx, edx
        jz aligned
        inc eax
        aligned:

        mov [DAP.sector_count], ax

        movzx eax, word [es:%2 + %3 * DIRECTORY_ENTRY_SIZE + CLUSTER_LOW_ENTRY_OFFSET]
        sub   eax, RESERVED_CLUSTER_COUNT
        movzx edx, byte [sectors_per_cluster]
        mul edx
        add eax, [data_offset_in_sectors]

        push ax
        mov ax, %1
        mov es, ax
        mov di, 0x7C00
        pop ax
        mov dl, [boot_drive]

        call read_disk_lba
%endmacro

; can read more than 64KB
; void read_directory_file_extended(dir_segment, index, filename)
%macro read_directory_file_extended 3
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
        mov ax, %1
        mov es, ax

        mov eax, [es:%2 * DIRECTORY_ENTRY_SIZE + FILESIZE_ENTRY_OFFSET]
        xor ebx, ebx
        xor edx, edx
        mov bx, [bytes_per_sector]
        div ebx
        test edx, edx
        jz aligned
        inc eax
        aligned:
        mov ecx, eax

        movzx eax, word [es:%2 * DIRECTORY_ENTRY_SIZE + CLUSTER_LOW_ENTRY_OFFSET]
        sub   eax, RESERVED_CLUSTER_COUNT
        movzx edx, byte [sectors_per_cluster]
        mul edx
        add eax, [data_offset_in_sectors]
        read_file_part:
            cmp ecx, MAX_SECTOR_COUNT_PER_READ
            jl read_rest ; under the limit

        read_batch:
            sub ecx, MAX_SECTOR_COUNT_PER_READ
            xor di, di
            mov [DAP.sector_count], word MAX_SECTOR_COUNT_PER_READ
            mov dl, [boot_drive]
            call read_disk_lba
            add eax, MAX_SECTOR_COUNT_PER_READ

            push ax
            mov ax, 65024
            mov di, ax
            pop ax

            mov [DAP.sector_count], word SECTOR_OVERFLOW_COUNT
            mov dl, [boot_drive]
            call read_disk_lba
            add eax, SECTOR_OVERFLOW_COUNT

            push ax
            mov ax, es
            add ax, 4096
            mov es, ax
            pop ax

            jmp read_file_part

        read_rest:
            mov [DAP.sector_count], cx
            mov dl, [boot_drive]
            xor di, di

            call read_disk_lba
%endmacro

%endif ; _COMMON_MACROS_INC_
