%ifndef _COMMON_MACROS_INC_
%define _COMMON_MACROS_INC_

CR: equ 0x0D
LF: equ 0x0A

FAT_DIRECTORY_SEGMENT:     equ 0x2000
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

SMAP:                      equ 0x0534D4150
E820:                      equ 0xE820
EXTENDED_ATTRIBUTE_OFFSET: equ 20
VALID_ENTRY:               equ 0b1
ACPI_ATTIRBUTES_SIZE:      equ 4
ENTRY_SIZE:                equ 20
FULL_ENTRY_SIZE:           equ ENTRY_SIZE + ACPI_ATTIRBUTES_SIZE
LENGTH_LOWER_OFFSET:       equ 8
LENGTH_HIGHER_OFFSET:      equ 12

GET_VESA_INFO:             equ 0x4F00
GET_VBE_MODE_INFO:         equ 0x4F01
SET_VBE_MODE:              equ 0x4F02
GET_EDID:                  equ 0x4f15
LINEAR_FRAMEBUFFER_BIT:    equ (1 << 14)
ATTRIBUTE_LINEAR_FB:       equ (1 << 7)
VESA_SUCCESS:              equ 0x004F
END_OF_MODE_LIST:          equ 0xFFFF

REAL_MODE_CODE:            equ 0x18
REAL_MODE_DATA:            equ 0x20

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

%macro switch_to_protected 0
    cli

    lgdt [gdt_entry]

    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    jmp gdt_ptr.code_32:%%.reset_selector

    BITS 32
    %%.reset_selector:
        ; setup segments
        mov ax,  gdt_ptr.data_32
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        mov esp, 0x30000
%endmacro

%macro switch_to_real 0

    cli
    jmp gdt_ptr.code_16:%%.reset_selector

    %%.reset_selector:
    BITS 16
    mov eax, gdt_ptr.data_16
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax

    mov eax, cr0
    xor eax, 1
    mov cr0, eax

    jmp 0x0:%%.actually_reset_selector

    %%.actually_reset_selector:
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov sp, 0x7C00
        sti
%endmacro

; can read into the entire 4GB memory space and potentially unlimited file sizes
; void read_directory_file_protected(at_address, index, filename)
%macro read_directory_file_protected 3
%%read_directory_file_protected:
    ; check if this file is what we're looking for
    mov cx, FAT_FILENAME_LENGTH
    mov ax, FAT_DIRECTORY_SEGMENT
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
        mov ax, FAT_DIRECTORY_SEGMENT
        mov es, ax

        mov eax, [es:%2 * DIRECTORY_ENTRY_SIZE + FILESIZE_ENTRY_OFFSET]
        xor edx, edx
        movzx ebx, word [bytes_per_sector]
        div ebx
        test edx, edx
        jz aligned
        inc eax
        aligned:
        mov [sectors_to_read], eax
        mov [memory_offset], dword %1

        movzx eax, word [es:%2 * DIRECTORY_ENTRY_SIZE + CLUSTER_LOW_ENTRY_OFFSET]
        sub   eax, RESERVED_CLUSTER_COUNT
        movzx edx, byte [sectors_per_cluster]
        mul edx
        add eax, [data_offset_in_sectors]
        mov [sector_offset], eax

        read_one:
            xor ax, ax
            mov es, ax
            mov [DAP.sector_count], word 1
            mov dl, [boot_drive]
            mov eax, [sector_offset]

            mov di, kernel_sector
            call read_disk_lba

        copy_protected:
            switch_to_protected
            xor eax, eax
            mov ax, kernel_sector
            mov esi, eax
            mov edi, [memory_offset]
            mov ecx, SECTOR_SIZE
            rep movsb
            add [memory_offset],   dword SECTOR_SIZE
            add [sector_offset],   dword 1
            sub [sectors_to_read], dword 1
            switch_to_real
            cmp dword [sectors_to_read], 0
            jg read_one
%endmacro

; void retrieve_memory_map(into_buffer, out_count)
%macro retrieve_memory_map 2
%%retrieve_memory_map:
    mov di, %1
    xor ebx, ebx
    xor bp, bp
    mov edx, SMAP
    mov eax, E820
    mov [es:di + EXTENDED_ATTRIBUTE_OFFSET], dword VALID_ENTRY
    mov ecx, FULL_ENTRY_SIZE
    int 0x15
    jc %%.on_unsupported
    mov edx, SMAP
    cmp eax, edx
    jne %%.on_unsupported
    test ebx, ebx
    je %%.on_unsupported
    jmp %%.validate_entry

    %%.next:
        mov eax, E820
        mov [es:di + EXTENDED_ATTRIBUTE_OFFSET], dword VALID_ENTRY
        mov ecx, FULL_ENTRY_SIZE
        int 0x15
        jc %%.end_of_list
        mov edx, SMAP

    %%.validate_entry:
        jcxz %%.skip_entry
        cmp cl, ENTRY_SIZE
        jbe %%.short_entry
        test [es:di + EXTENDED_ATTRIBUTE_OFFSET], byte VALID_ENTRY
        je %%.skip_entry

    %%.short_entry:
        mov ecx, [es:di + LENGTH_LOWER_OFFSET]
        or ecx,  [es:di + LENGTH_HIGHER_OFFSET]
        jz %%.skip_entry
        inc bp
        add di, FULL_ENTRY_SIZE

    %%.skip_entry:
        test ebx, ebx
        jne %%.next

    %%.end_of_list:
        mov [%2], bp
        clc
        jmp %%.done

    %%.on_unsupported:
        mov si, e820_unsupported
        call write_string
        call reboot

    %%.done:
%endmacro

; void set_video_mode(width_at_least, height_at_least, bpp_at_least)
; sets the video mode to [native >= mode >= requested]
%macro set_video_mode 3
%%set_video_mode:
    push es
    mov  ax, GET_VESA_INFO
    mov  di, vbe_info
    int 0x10
    pop es

    cmp ax, VESA_SUCCESS
    jne %%.error

    mov ax, [vbe_info.modes_segment]
    mov fs, ax
    mov si, [vbe_info.modes_offset]

    push es
    mov  ax, GET_EDID
    mov  bl, 0x01
    xor  cx, cx
    xor  dx, dx
    mov  di, edid
    int 0x10
    pop es

    cmp ax, VESA_SUCCESS
    jne %%.sanity_check ; Get EDID fucntion is unsupported, so we assume 1024x768 is native

    movzx ax, byte [edid.horizontal_active_lower]
    mov [native_width], ax
    movzx bx, byte [edid.horizontal_active_higher]
    and   bl, 0xF0
    shl   bx, 4
    add [native_width], bx

    movzx ax, byte [edid.vertical_active_lower]
    mov [native_height], ax
    movzx bx, byte [edid.vertical_active_higher]
    and   bl, 0xF0
    shl   bx, 4
    add [native_height], bx

%%.sanity_check:
    ; make sure that requested resolution is <= native
    mov ax, %1
    cmp ax, [native_width]
    jg %%.error

    mov ax, %2
    cmp ax, [native_height]
    jg %%.error

%%.check_next_mode:
    mov cx, [fs:si]
    add si, 2

    cmp cx, END_OF_MODE_LIST
    je %%.set_best_mode

    push es
    mov ax, GET_VBE_MODE_INFO
    mov di, vbe_mode_info
    int 0x10
    pop es

    cmp ax, VESA_SUCCESS
    jne %%.error

    mov ax, [vbe_mode_info.width]
    cmp ax, %1
    jl %%.check_next_mode
    cmp ax, [native_width]
    jg %%.check_next_mode ; too big

    mov ax, [vbe_mode_info.height]
    cmp ax, %2
    jl %%.check_next_mode
    cmp ax, [native_height]
    jg %%.check_next_mode ; too big

    mov al, [vbe_mode_info.bpp]
    cmp al, %3
    jl %%.check_next_mode

    mov ax, [vbe_mode_info.attributes]
    and ax, ATTRIBUTE_LINEAR_FB
    jz %%.check_next_mode

    ; found a potentially good mode, compare to the best
    mov ax, [vbe_mode_info.width]
    cmp ax, [video_mode.width]
    jl %%.check_next_mode

    mov ax, [vbe_mode_info.height]
    cmp ax, [video_mode.height]
    jl %%.check_next_mode

    mov al, [vbe_mode_info.bpp]
    cmp al, [video_mode.bpp]
    jl %%.check_next_mode

    ; this is the best one so far, record it
    mov ax, [vbe_mode_info.width]
    mov [video_mode.width], ax

    mov ax, [vbe_mode_info.height]
    mov [video_mode.height], ax

    mov al, [vbe_mode_info.bpp]
    mov [video_mode.bpp], al

    mov eax, [vbe_mode_info.framebuffer]
    mov [video_mode.framebuffer], eax

    mov ax, [vbe_mode_info.pitch]
    mov [video_mode.pitch], ax

    mov [best_mode], cx

    jmp %%.check_next_mode

%%.set_best_mode:
    mov  bx, [best_mode]
    test bx, bx
    jz %%.error

    push es
    mov ax, SET_VBE_MODE
    or  bx, LINEAR_FRAMEBUFFER_BIT
    mov di, 0
    int 0x10
    pop es

    cmp ax, VESA_SUCCESS
    je %%.done

%%.error:
    mov si, vesa_failure_msg
    call write_string
    call reboot

%%.done:

%endmacro

%endif ; _COMMON_MACROS_INC_
