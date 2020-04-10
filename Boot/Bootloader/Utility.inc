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

reboot_msg db "Press any key to reboot.", CR, LF, 0x0

; [[noreturn]] void reboot()
reboot:
    mov si, reboot_msg
    call write_string
    xor ax, ax
    int 0x16
    jmp word 0xFFFF:0x0000

; void reset_disk_system(dl disk)
reset_disk_system:
    xor ax, ax
    int 0x13
    ret

disk_error_msg db "Error while reading disk.", CR, LF, 0x0

; [[noreturn]] void on_boot_failed()
on_boot_failed:
    mov si, disk_error_msg
    call write_string
    call reboot

; void read_sector(es:bx into, ax LBA)
read_sector:
    xor cx, cx

.try_read:
    push ax
    push cx
    push bx

    mov bx, [SectorsPerTrack]
    xor dx, dx
    div bx

    inc dx
    mov cl, dl

    mov bx, [NumberOfSides]
    xor dx, dx
    div bx

    mov ch, al
    xchg dl, dh

    mov ax, 0x0201
    mov dl, [BootDrive]
    pop bx
    int 0x13
    jc .on_read_failed

    pop cx
    pop ax
    ret

.on_read_failed:
    pop cx
    inc cx
    cmp cx, [MAX_READ_TRIES]
    je on_boot_failed

    call reset_disk_system
    pop ax
    jmp .try_read

root_dir_size   dw 0
root_dir_sector dw 0
