BITS 16

main:
    jmp short start

%include "BPB.inc"
%include "CommonMacros.inc"

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

    mov si, success_msg
    call write_string

    ; ---- try enable the A20 line ----
    enable_a20:
        mov si, a20_msg
        call write_string

        ; check if its already enabled
        call is_a20_enabled
        cmp ax, 1
        je a20_success

        ; try using the BIOS interrupt 15
        call try_enable_a20_using_bios
        call is_a20_enabled
        cmp ax, 1
        je a20_success

        ; try using the 8042 controller
        call try_enable_a20_using_8042
        call is_a20_enabled
        cmp ax, 1
        je a20_success

        ; try using fast gate
        call try_enable_a20_using_fast_gate
        call is_a20_enabled
        cmp ax, 1
        je a20_success

    a20_failure:
        mov si, failure_msg
        call write_string
        call reboot

    a20_success:
        mov si, success_msg
        call write_string
        jmp $

%include "Common.inc"
%include "LoaderUtils.inc"

BPB_OFFSET: equ 0x7C03
KERNEL_SEGMENT: equ 0x2000
BPB_SIZE: equ 34

welcome_msg db "Reading kernel from disk...", CR, LF, 0
a20_msg     db "Trying to enable the A20 line...", CR, LF, 0
success_msg db "Done!", CR, LF, 0
failure_msg db "Failed!", CR, LF, 0

kernel_file db "UKLoaderbin"
kernel_file_cluster dw 0
