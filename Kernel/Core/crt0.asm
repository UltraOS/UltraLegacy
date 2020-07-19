%define KERNEL_ENTRYPOINT _ZN6kernel3runENS_9MemoryMapE

extern KERNEL_ENTRYPOINT

%define KB (1024)
%define MB (1024 * KB)

KERNEL_SIZE:    equ 3 * MB

PAGE_SIZE:      equ 4096
ENTRY_COUNT:    equ 1024
VIRTUAL_ORIGIN: equ 0xC0000000
KERNEL_ORIGIN:  equ 0xC0100000
PRESENT:        equ 0b01
READWRITE:      equ 0b10
GLOBAL_PAGE:    equ (1 << 8)
ENTRY_SIZE:     equ 4
EM_BIT:         equ (1 << 2)
TS_BIT:         equ (1 << 3)
GLOBAL_BIT:     equ (1 << 7)

HEAP_ENTRY_COUNT: equ 1024 ; 4MB of kernel heap

WRITE_PROTECT: equ (0b1 << 16)
PAGING:        equ (0b1 << 31)

%define TO_PHYSICAL(virtual) (virtual - VIRTUAL_ORIGIN)

section .bss
; can't use PAGE_SIZE here because of a NASM bug
; it simply ignores this if it's a macro
align 4096

global kernel_page_directory
kernel_page_directory:
    resb PAGE_SIZE

global kernel_page_table
kernel_page_table:
    resb PAGE_SIZE

kernel_heap_table:
    resb PAGE_SIZE

kernel_quickmap_table:
    resb PAGE_SIZE

; Current memory layout (physical)
; 0MB -> 1MB unused (bootloader and bios stuff)
; 1MB -> 4MB kernel
; 4MB -> 8MB kernel heap
; 8MB -> 4GB unsued

; Current memory layout (virtual)
; 0x00000000 -> 0xBFFFFFFF user space
; 0xC0000000 -> 0xC0100000 unused (vga memory is here somewhere)
; 0xC0100000 -> 0xC03FFFFF kernel
; 0xC0400000 -> 0xC07FFFFF kernel heap
; 0xC0800000 -> 0xFFBFFFFF unused
; 0xFFC00000 -> 0xFFFFFFFF reserved for recursive paging

section .entry

extern section_bss_begin
extern section_bss_end

global start
start:
    ; zero section bss
    mov ecx, TO_PHYSICAL(section_bss_end)
    sub ecx, TO_PHYSICAL(section_bss_begin)
    mov edi, TO_PHYSICAL(section_bss_begin)
    mov ebp, eax ; save the eax
    mov eax, 0
    rep stosb
    mov eax, ebp

    ; TODO: actually check if we have one.
    ; and maybe move this out into a CPU class in C++
    mov edx, cr0
    and edx, ~(EM_BIT | TS_BIT)
    mov cr0, edx
    fninit ; initialize the FPU

    mov ecx, cr4
    or  ecx, GLOBAL_BIT
    mov cr4, ecx

    mov edi, TO_PHYSICAL(kernel_page_table)
    mov esi, 0x00000000
    mov ecx, ENTRY_COUNT

    map_one:
        mov edx, esi
        or  edx, (PRESENT | READWRITE | GLOBAL_PAGE)
        mov [edi], edx

        add esi, PAGE_SIZE
        add edi, ENTRY_SIZE
        sub ecx, 1

        cmp ecx, 0
        jg map_one

    ; TODO: make sure we actually have this much RAM free
    mov edi, TO_PHYSICAL(kernel_heap_table)
    mov esi, TO_PHYSICAL(KERNEL_ORIGIN + KERNEL_SIZE)
    mov ecx, HEAP_ENTRY_COUNT

    map_one_more:
        mov edx, esi
        or  edx, (PRESENT | READWRITE | GLOBAL_PAGE)
        mov [edi], edx

        add esi, PAGE_SIZE
        add edi, ENTRY_SIZE
        sub ecx, 1

        cmp ecx, 0
        jg map_one_more

    ; identity mapping to enable paging
    mov [TO_PHYSICAL(kernel_page_directory)], dword (TO_PHYSICAL(kernel_page_table) + (PRESENT | READWRITE))

    ; actual mapping
    mov [TO_PHYSICAL(kernel_page_directory + 768 * ENTRY_SIZE)], dword (TO_PHYSICAL(kernel_page_table) + (PRESENT | READWRITE))

    ; kernel heap
    mov [TO_PHYSICAL(kernel_page_directory + 769 * ENTRY_SIZE)], dword (TO_PHYSICAL(kernel_heap_table) + (PRESENT | READWRITE))

    ; kernel quickmap table
    mov [TO_PHYSICAL(kernel_page_directory + 770 * ENTRY_SIZE)], dword (TO_PHYSICAL(kernel_quickmap_table) + (PRESENT | READWRITE))

    ; recursive paging mapping
    mov [TO_PHYSICAL(kernel_page_directory + 1023 * ENTRY_SIZE)], dword (TO_PHYSICAL(kernel_page_directory) + (PRESENT | READWRITE))

    mov ecx, TO_PHYSICAL(kernel_page_directory)
    mov cr3, ecx

    mov ecx, cr0
    or  ecx, (WRITE_PROTECT | PAGING)
    mov cr0, ecx

    mov ecx, higher_half
    jmp ecx

    higher_half:
        ; remove the identity mapping here
        mov [kernel_page_directory], dword 0x00000000

        ; flush TLB
        mov ecx, cr3
        mov cr3, ecx
        invlpg [0x00100000]

        ; Set up the kernel stack
        mov esp, kernel_stack_begin

        ; entry count
        push ebx

        ; memory map pointer
        add eax, VIRTUAL_ORIGIN
        push eax

    ; Jump into kernel main
    call KERNEL_ENTRYPOINT
hang:
    cli
    hlt
    jmp hang

section .bss
align 16
kernel_stack_end:
    resb 16384
global kernel_stack_begin
kernel_stack_begin:

section .magic
    global magic_string
    ; a magic "CRC" string to verify the integrity
    magic_string: db "MAGIC_PRESENT", 0
