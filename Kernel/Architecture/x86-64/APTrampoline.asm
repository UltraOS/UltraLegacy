BITS 16

section .text

VIRTUAL_ORIGIN: equ 0xC0000000
ORIGIN:         equ 0x1000
ORIGIN_SEGMENT: equ (ORIGIN >> 4)
TEMPO_STACK:    equ 0x7000
PRESENT:        equ 0b01
READWRITE:      equ 0b10
WRITE_PROTECT:  equ (0b1 << 16)
PAGING:         equ (0b1 << 31)

%define ADDR_OF(label) label - application_processor_entrypoint
%define TO_PHYSICAL(virtual) (virtual - VIRTUAL_ORIGIN)

global application_processor_entrypoint:
application_processor_entrypoint:
    cli
    hlt

global alive
alive:
    db 0

global allowed_to_boot
allowed_to_boot:
    db 0
