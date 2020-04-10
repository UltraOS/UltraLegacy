jmp short start

%include "BPB.inc"
%include "Macros.inc"

start:
    mov si, welcome_msg
    call write_string

    ; steal BPB from the bootloader
    push ds
    xor ax, ax
    mov ds, ax
    mov si, BPB_OFFSET
    mov di, BPB
    mov cx, BPB_SIZE
    rep movsb
    pop ds

    ; find the kernel file cluster
    find_file_on_disk kernel_file, KERNEL_SEGMENT
    mov [kernel_file_cluster], ax

    ; read the kernel file into its segment
    read_file_from_disk [kernel_file_cluster], KERNEL_SEGMENT

    ; reset the disk system
    mov dl, [BootDrive]
    call reset_disk_system

    mov si, done_msg
    call write_string

    ; enable the A20 line
    mov si, a20_msg
    call write_string
    jmp $

%include "Utility.inc"

BPB_OFFSET: equ 0x7C03
KERNEL_SEGMENT: equ 0x2000
BPB_SIZE: equ 34

welcome_msg db "Reading kernel from disk...", CR, LF, 0
a20_msg     db "Trying to enable the A20 line...", CR, LF, 0
done_msg    db "Done!", CR, LF, 0

kernel_file db "UKLoaderbin"
kernel_file_cluster dw 0
