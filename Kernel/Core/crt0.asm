%define KERNEL_ENTRYPOINT _ZN6kernel3runENS_9MemoryMapE

extern KERNEL_ENTRYPOINT

section .bss

PAGE_SIZE:      equ 4096
ENTRY_COUNT:    equ 1024
VIRTUAL_ORIGIN: equ 0xC0000000
PRESENT:        equ 0b01
READWRITE:      equ 0b10
ENTRY_SIZE:     equ 4

WRITE_PROTECT: equ (0b1 << 16)
PAGING:        equ (0b1 << 31)

%define TO_PHYSICAL(virtual) (virtual - VIRTUAL_ORIGIN)

section .data
; can't use PAGE_SIZE here because of a NASM bug
; it simply ignores this if it's a macro
align 4096

boot_page_directory:
    times PAGE_SIZE db 0

identity_lower:
    times PAGE_SIZE db 0

boot_page_table:
    times PAGE_SIZE db 0

section .entry

global start
start:
    mov edi, boot_page_table
    mov edi, TO_PHYSICAL(boot_page_table)
    mov esi, 0x00000000
    mov ecx, ENTRY_COUNT

    map_one:
        mov edx, esi
        or  edx, (PRESENT | READWRITE)
        mov [edi], edx

        add esi, PAGE_SIZE
        add edi, ENTRY_SIZE
        sub ecx, 1

        cmp ecx, 0
        jg map_one

    ; identity
    mov [TO_PHYSICAL(boot_page_directory)], dword (TO_PHYSICAL(boot_page_table) + (PRESENT | READWRITE))

    ; actual mapping
    mov [TO_PHYSICAL(boot_page_directory + 768 * ENTRY_SIZE)], dword (TO_PHYSICAL(boot_page_table) + (PRESENT | READWRITE))

    mov ecx, TO_PHYSICAL(boot_page_directory)
    mov cr3, ecx

    mov ecx, cr0
    or  ecx, (WRITE_PROTECT | PAGING)
    mov cr0, ecx

    mov ecx, higher_half
    jmp ecx

    higher_half:
        ; TODO: delete this once I have GDT in C++
        ; keep the identity mapping for the first MB
        mov edi, identity_lower
        mov esi, 0x00000000
        mov ecx, 256 ; 1 MB (256 * 4096)

        map_identity:
            mov edx, esi
            or  edx, (PRESENT | READWRITE)
            mov [edi], edx

            add esi, PAGE_SIZE
            add edi, ENTRY_SIZE
            sub ecx, 1

            cmp ecx, 0
            jg map_identity

        mov [boot_page_directory], dword TO_PHYSICAL(identity_lower) + (PRESENT | READWRITE)

        ; flush TLB
        mov ecx, cr3
        mov cr3, ecx

        ; Set up the kernel stack
        mov esp, kernel_stack_begin

        ; entry count
        push ebx

        ; memory map pointer
        ; TODO: reenable once I don't identity map the 1st MB
        ;add eax, VIRTUAL_ORIGIN
        push eax

    ; Jump into kernel main
    call KERNEL_ENTRYPOINT
hang:
    cli
    hlt
    jmp hang

section .kernel_stack
align 16
kernel_stack_end:
    times 16384 db 0
kernel_stack_begin:

section .magic
    global magic_string
    ; a magic "CRC" string to verify the integrity
    magic_string: db "MAGIC_PRESENT", 0
