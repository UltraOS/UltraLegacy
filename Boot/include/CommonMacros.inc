%ifndef _COMMON_MACROS_INC_
%define _COMMON_MACROS_INC_

CR: equ 0x0D
LF: equ 0x0A

CLUSTER_LOW_ENTRY_OFFSET:  equ 26
FILESIZE_ENTRY_OFFSET:     equ 28
RESERVED_CLUSTER_COUNT:    equ 2
FAT_FILENAME_LENGTH:       equ 11
VBR_ORIGIN:                equ 0x7C00
BPB_OFFSET:                equ 11
BPB_SIZE:                  equ 79 ; FAT32

; magic breakpoint in Bochs
%define BP xchg bx, bx

; void read_root_directory(boot_dirve, into_segment)
%macro read_root_directory 2
read_root_dir:
    ; read the first sector of the root directory
    mov dl, %1
    mov ax, %2
    mov es, ax
    xor di, di
    mov eax, [RootDirCluster]
    sub eax, RESERVED_CLUSTER_COUNT
    add eax, [data_offset_in_sectors]
    call read_disk_lba
%endmacro

%endif ; _COMMON_MACROS_INC_
