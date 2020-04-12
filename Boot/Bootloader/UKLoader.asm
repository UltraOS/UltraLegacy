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

    ; ---- set up IDT and GDT ----
    mov si, idt_gdt_msg
    call write_string
    xor ax, ax
    mov es, ax
    mov di, ax
    mov cx, 2048
    rep stosb ; 256 empty IDT entries

    ; NULL descriptor
    mov cx, 4
    rep stosw

    ; code segment descriptor
    mov [es:di],     word 0xFFFF ; limit
    mov [es:di + 2], word 0x0000 ; base
    mov [es:di + 4], byte 0x00   ; base
    mov [es:di + 5], byte 0x9A   ; access
    mov [es:di + 6], byte 0xCF   ; granularity
    mov [es:di + 7], byte 0x00   ; base
    add di, 8

    ; data segment descriptor
    mov [es:di],     word 0xFFFF ; limit
    mov [es:di + 2], word 0x0000 ; base
    mov [es:di + 4], byte 0x00   ; base
    mov [es:di + 5], byte 0x92   ; access
    mov [es:di + 6], byte 0xCF   ; granularity
    mov [es:di + 7], byte 0x00   ; base

    cli

    lgdt [gdt_entry]
    lidt [idt_entry]

    ; switch to protected mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    ; clear prefetch queue
    jmp clear_prefetch
    clear_prefetch:

    ; setup segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x30000

    jmp $ ; we don't actually have a kernel yet

    ; jump to the kernel
    db 0x66
    db 0xEA
    dd KERNEL_FLAT_ADDRESS
    dw 0x0008

%include "Common.inc"
%include "LoaderUtils.inc"

idt_entry:
    dw 2048
    dd 0

gdt_entry:
    dw 24
    dd 2048

BPB_OFFSET: equ 0x7C03
KERNEL_SEGMENT: equ 0x2000
KERNEL_FLAT_ADDRESS: equ 0x20000 ; KERNEL_SEGMENT << 4
BPB_SIZE: equ 34

welcome_msg db "Reading kernel from disk...", CR, LF, 0
a20_msg     db "Enabling the A20 line...", CR, LF, 0
idt_gdt_msg db "Setting up IDT and GDT...", CR, LF, 0
success_msg db "Done!", CR, LF, 0
failure_msg db "Failed!", CR, LF, 0

kernel_file db "UKLoaderbin"
kernel_file_cluster dw 0

BITS 32
kernel:
BRKP
jmp $