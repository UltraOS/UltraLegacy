BITS 16

%include "CommonMacros.inc"
MBR_ORIGINAL_ADDRESS:  equ 0x7C00
VBR_LOAD_ADDRESS:      equ MBR_ORIGINAL_ADDRESS
MBR_RELOCATED_ADDRESS: equ 0x0600
ACTIVE_PARTITION_FLAG: equ 0b10000000
PARTITION_ENTRY_SIZE:  equ 16
PARTITION_COUNT:       equ 4
MBR_SIZE_IN_BYTES:     equ 512
PARTITION_LBA_OFFSET:  equ 8

; fake origin here to make it easier for us
ORG MBR_RELOCATED_ADDRESS

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, MBR_ORIGINAL_ADDRESS

; Relocate ourselves to give space to the VBR
relocate:
    push ds
    mov ds, ax
    mov si, MBR_ORIGINAL_ADDRESS
    mov di, MBR_RELOCATED_ADDRESS
    mov cx, MBR_SIZE_IN_BYTES
    rep movsb
    pop ds
    jmp 0x0:main ; force a long jump here

; actual main that we get to after we've relocated
main:
    mov [drive_number], dl
    ; pick a partition
    mov bx, partition1
    mov cx, 1
    pick_a_partiton:
        cmp byte [bx], ACTIVE_PARTITION_FLAG
        je partition_found
        add bx, PARTITION_ENTRY_SIZE
        cmp cx, PARTITION_COUNT
        je on_no_partition_found
        inc cx
        jmp pick_a_partiton

    partition_found:
        mov [selected_partition], bx
        ; We use LBA addressing for any partition
        ; Let's check if our bios supports that
        mov ah, 0x41
        mov bx, 0x55aa
        int 0x13
        jnc load_lba_partition

        mov si, no_lba_support_error
        call write_string
        call reboot

    load_lba_partition:
        mov bx, [selected_partition]
        add bx, PARTITION_LBA_OFFSET
        mov ax, 0x0
        mov es, ax
        mov eax, [bx]
        mov di, VBR_LOAD_ADDRESS
        call read_disk_lba
        jmp transfer_control_to_vbr

    transfer_control_to_vbr:
        mov bx, [selected_partition]
        mov dl, [drive_number]
        jmp 0x0:VBR_LOAD_ADDRESS

    on_no_partition_found:
        mov si, no_partition_error
        call write_string
        call reboot

selected_partition:   dw 0
drive_number:         db 0x0
no_partition_error:   db "No bootable partiton found!", CR, LF, 0
no_lba_support_error: db "This BIOS doesn't support LBA disk access!", CR, LF, 0

%include "Common.inc"

times 446-($-$$) db 0

partition1:
    .status:                db 0
    .head_begin:            db 0
    .sector_cylinder_begin: db 0
    .cylinder_begin:        db 0
    .partition_type:        db 0
    .head_end:              db 0
    .sector_cylinder_end:   db 0
    .cylinder_end:          db 0
    .lba_offset:            dd 0
    .sector_count:          dd 0
partition2:
    .status:                db 0
    .head_begin:            db 0
    .sector_cylinder_begin: db 0
    .cylinder_begin:        db 0
    .partition_type:        db 0
    .head_end:              db 0
    .sector_cylinder_end:   db 0
    .cylinder_end:          db 0
    .lba_offset:            dd 0
    .sector_count:          dd 0
partition3:
    .status:                db 0
    .head_begin:            db 0
    .sector_cylinder_begin: db 0
    .cylinder_begin:        db 0
    .partition_type:        db 0
    .head_end:              db 0
    .sector_cylinder_end:   db 0
    .cylinder_end:          db 0
    .lba_offset:            dd 0
    .sector_count:          dd 0
partition4:
    .status:                db 0
    .head_begin:            db 0
    .sector_cylinder_begin: db 0
    .cylinder_begin:        db 0
    .partition_type:        db 0
    .head_end:              db 0
    .sector_cylinder_end:   db 0
    .cylinder_end:          db 0
    .lba_offset:            dd 0
    .sector_count:          dd 0

boot_signature dw 0xAA55