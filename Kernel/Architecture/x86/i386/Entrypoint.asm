%define KERNEL_ENTRYPOINT _ZN6kernel3runEPNS_13LoaderContextE

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
ENTRY_SIZE:     equ 4

WRITE_PROTECT: equ (0b1 << 16)
PAGING:        equ (0b1 << 31)

%define TO_PHYSICAL(virtual) (virtual - VIRTUAL_ORIGIN)

section .bss
; can't use PAGE_SIZE here because of a NASM bug
; it simply ignores this if it's a macro
align 4096

global kernel_base_table
kernel_base_table:
    resb PAGE_SIZE

global kernel_page_table
kernel_page_table:
    resb PAGE_SIZE

kernel_heap_table:
    resb PAGE_SIZE

kernel_quickmap_table:
    resb PAGE_SIZE

global memory_map_entries_buffer
memory_map_entries_buffer:
    resb PAGE_SIZE

global bsp_kernel_stack_end
bsp_kernel_stack_end:
    resb 32768
bsp_kernel_stack_begin:

; Physical memory layout
; 0MB -> 1MB unused (bootloader and bios stuff)
; 1MB -> 4MB kernel
; 4MB -> 4GB generic free memory  (4 MB -> 8MB range is likely the initial kernel heap block)

; Virtual memory layout
; 0x00000000 -> 0xBFFFFFFF user space
; 0xC0000000 -> 0xC0100000 unused (direct mapping of the first MB)
; 0xC0100000 -> 0xC03FFFFF kernel (direct mapping of 1MB -> 4MB)
; 0xC0400000 -> 0xC07FFFFF first kernel heap block
; 0xC0800000 -> 0xC087FFFF kernel quick-map range (used by the MM to access random physical memory)
; 0xC0880000 -> 0xFFBFFFFF kernel address space free for allocation for any kernel thread
; 0xFFC00000 -> 0xFFFFFFFF reserved for recursive paging

section .entry

extern section_bss_begin
extern section_bss_end

; set_directory_entry_to(pd_index, phys_addr)
%macro set_directory_entry 2
mov [TO_PHYSICAL(kernel_base_table + %1 * ENTRY_SIZE)], dword (TO_PHYSICAL(%2) + (PRESENT | READWRITE))
%endmacro

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

    ; FPU presence bit
    EM_BIT: equ (1 << 2)

    ; trap x87 instructions
    TS_BIT: equ (1 << 3)

    ; native exception handling
    NE_BIT: equ (1 << 5)

    mov edx, cr0
    and edx, ~(EM_BIT | TS_BIT)
    or  edx, NE_BIT
    mov cr0, edx

    ; set up a direct + identity mapping for the kernel page table
    ; e.g virtual 0x00000000 points to physical 0x00000000
    ; and virtual 0xC0000000 points to physical 0x00000000
    mov edi, TO_PHYSICAL(kernel_page_table)
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

    ; identity mapping to enable paging
    set_directory_entry 0, kernel_page_table

    ; the kernel image direct mapping
    set_directory_entry 768, kernel_page_table

    ; kernel heap
    set_directory_entry 769, kernel_heap_table

    ; kernel quickmap table
   set_directory_entry 770, kernel_quickmap_table

    ; recursive paging mapping
    set_directory_entry 1023, kernel_base_table

    mov ecx, TO_PHYSICAL(kernel_base_table)
    mov cr3, ecx

    mov ecx, cr0
    or  ecx, (WRITE_PROTECT | PAGING)
    mov cr0, ecx

    mov ecx, higher_half
    jmp ecx

    higher_half:
        ; remove the identity mapping here
        mov [kernel_base_table], dword 0x00000000

        ; flush TLB
        mov ecx, cr3
        mov cr3, ecx

        ; Set up the kernel stack
        mov esp, bsp_kernel_stack_begin

        ; align the stack to 16 for the ABI
        sub esp, 12 ; 0 + 12 + boot context = 16

        ; boot context pointer
        push eax

    xor ebp, ebp
    ; Jump into kernel main
    call KERNEL_ENTRYPOINT
hang:
    cli
    hlt
    jmp hang

section .magic
    global magic_string
    ; a magic "CRC" string to verify the integrity
    magic_string: db "MAGIC_PRESENT", 0
