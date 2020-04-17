BITS 32

; print a message to see if
; we successfully loaded
; into x86 protected mode
mov eax,      dword 0xB8000
mov [eax     ], byte 'X'
mov [eax + 1 ], byte 0x0A
mov [eax + 2 ], word '8'
mov [eax + 3 ], byte 0x0A
mov [eax + 4 ], word '6'
mov [eax + 5 ], byte 0x0A
mov [eax + 6 ], word ' '
mov [eax + 7 ], byte 0x0A
mov [eax + 8 ], word 'H'
mov [eax + 9 ], byte 0x0A
mov [eax + 10], word 'E'
mov [eax + 11], byte 0x0A
mov [eax + 12], word 'L'
mov [eax + 13], byte 0x0A
mov [eax + 14], word 'L'
mov [eax + 15], byte 0x0A
mov [eax + 16], word 'O'
mov [eax + 17], byte 0x0A

jmp $