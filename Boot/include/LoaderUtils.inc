%ifndef _LOADER_UTILS_INC_
%define _LOADER_UTILS_INC_

; ax is_a20_enabled()
; returns 1 if enabled, 0 otherwise
is_a20_enabled:
    pushf
    push ds
    push es
    push di
    push si

    cli

    xor ax, ax
    mov es, ax
    mov di, 0x0500

    not ax
    mov ds, ax
    mov si, 0x510

    mov al, [es:di]
    push ax

    mov al, [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    pop ax
    mov [ds:si], al

    pop ax
    mov [es:di], al

    mov ax, 0
    je .exit

    mov ax, 1

.exit:
    pop si
    pop di
    pop es
    pop ds
    popf
    ret

; void try_enable_a20_using_bios()
try_enable_a20_using_bios:
    pushf
    push ax
    mov ax, 0x2401
    int 0x15
    pop ax
    popf
    ret

; void wait_for_8042()
wait_for_8042_command:
    in al, 0x64
    test al, 2
    jnz wait_for_8042_command
    ret

; void wait_for_8042()
wait_for_8042_data:
    in al, 0x64
    test al, 1
    jz wait_for_8042_data
    ret

; void try_enable_a20_using_8042()
try_enable_a20_using_8042:
    pusha
    pushf

    cli

    call wait_for_8042_command
    mov al, 0xAD
    out 0x64, al

    call wait_for_8042_command
    mov al, 0xD0
    out 0x64, al

    call wait_for_8042_data
    in al, 0x60
    push ax

    call wait_for_8042_command
    mov al, 0xD1
    out 0x64, al

    call wait_for_8042_command
    pop ax
    or al, 2
    out 0x60, al

    call wait_for_8042_command
    mov al, 0xAE
    out 0x64, al

    call wait_for_8042_command

    sti

    popf
    popa

    ret

; void try_enable_a20_using_fast_gate()
try_enable_a20_using_fast_gate:
    push ax

    in al, 0x92
    or al, 2
    out 0x92, al

    pop ax
    ret

%endif ; _LOADER_UTILS_INC_