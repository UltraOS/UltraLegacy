BYTES_PER_DIR_ENTRY: equ 32
CLUSTER_ENTRY_OFFSET: equ 0x1A
MAX_READ_TRIES: equ 5

CR: equ 0x0D
LF: equ 0x0A

; void clear_screen()
%macro clear_screen 0
    mov ax, 0x0007
    int 0x10
%endmacro

; ax find_file_on_disk(file_name, read_segment)
%macro find_file_on_disk 2
%%find_file_on_disk:
    mov ax, %2
    mov es, ax

    mov ax, BYTES_PER_DIR_ENTRY
    xor dx, dx
    mul word [RootDirEntries]
    div word [BytesPerSector]
    mov cx, ax
    mov [root_dir_size], cx

    xor ax, ax
    mov al, [FATCount]
    mov bx, [FATSize]
    mul bx
    add ax, word [SectorsHidden]
    adc ax, word [SectorsHidden + 2]
    add ax, [SectorsReserved]
    mov [root_dir_sector], ax

%%.read_next_sector:
    push cx
    push ax
    xor bx, bx
    call read_sector

%%.check_next_entry:
    mov cx, 11
    mov di, bx
    mov si, %1
    repz cmpsb
    je %%.on_file_found
    add bx, BYTES_PER_DIR_ENTRY
    cmp bx, [BytesPerSector]
    jl %%.check_next_entry

    pop ax
    inc ax
    pop cx
    loopnz %%.read_next_sector
    jmp on_boot_failed

%%.on_file_found:
    mov ax, [es:bx + CLUSTER_ENTRY_OFFSET]
%endmacro

FAT_LOAD_SEGMENT: equ 0x0EE0

; void load_fat()
%macro load_fat 0
%%load_fat:
    mov ax, FAT_LOAD_SEGMENT
    mov es, ax

    mov ax, [SectorsReserved]
    add ax, word [SectorsHidden]
    adc ax, word [SectorsHidden + 2]

    mov cx, [FATSize]
    xor bx, bx
%%.load_next_fat:
    push cx
    push ax
    call read_sector
    pop ax
    pop cx
    inc ax
    add bx, [BytesPerSector]
    loopnz %%.load_next_fat
%endmacro

; void read_file_from_disk(cluster, load_into)
%macro read_file_from_disk 2
%%read_file_from_disk:
    mov ax, %2
    mov es, ax
    xor bx, bx
    mov cx, %1

%%.read_next_sector:
    mov ax, cx
    add ax, [root_dir_sector]
    add ax, [root_dir_size]
    sub ax, 2

    push cx
    call read_sector
    pop cx
    add bx, [BytesPerSector]

    push ds
    mov dx, FAT_LOAD_SEGMENT
    mov ds, dx

    mov si, cx
    mov dx, cx
    shr dx, 1
    add si, dx

    mov dx, [ds:si]
    test cx, 1
    jnz %%.read_next_even
    and dx, 0x0FFF
    jmp %%.read_next
%%.read_next_even:
    shr dx, 4
%%.read_next:
    pop ds
    mov cx, dx
    cmp cx, 0xFF8
    jl %%.read_next_sector
%endmacro
