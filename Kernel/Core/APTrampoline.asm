BITS 16

global application_processor_entrypoint:
application_processor_entrypoint:
    mov ax, 0
    mov ds, ax
    mov [ds:0x2000], word 0xDEAD

    idle_loop:
        cli
        hlt
        jmp idle_loop
