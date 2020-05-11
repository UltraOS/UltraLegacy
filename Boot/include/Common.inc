%ifndef _COMMON_INC_
%define _COMMON_INC_

; void write_string(ds:si string)
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

; modifies: eax, si
; expects: dl to be the drive letter, ds to be set to the current segment
; void read_disk_lba(u32 lba_start, u16 sector_count, u16 into_segment, u16 into_offset)
read_disk_lba:
.setup_dap:
    mov [DAP.sector_begin_low],  eax
    mov [DAP.read_into_segment], es
    mov [DAP.read_into_offset],  di
    mov ax, DAP
    mov si, ax
.read:
    mov ah, 0x42
    int 0x13
    jc .on_disk_read_failed
    ret
.on_disk_read_failed:
    mov si, disk_error
    call write_string
    call reboot

DAP_SIZE: equ 16
DAP:
    .size:              db DAP_SIZE
    .unused:            db 0x0
    .sector_count:      dw 0x1
    .read_into_offset:  dw 0x0
    .read_into_segment: dw 0x0
    .sector_begin_low:  dd 0x0
    .sector_begin_high: dd 0x0

; [[noreturn]] void reboot()
reboot:
    mov si, reboot_msg
    call write_string
    xor ax, ax
    int 0x16
    jmp word 0xFFFF:0x0000

reboot_msg: db "Press any key to reboot...", CR, LF, 0x0
disk_error: db "Error reading disk!", CR, LF, 0

%endif ; _COMMON_INC_
