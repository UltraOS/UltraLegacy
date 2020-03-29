mov ax, 0x1000
mov ds, ax
loop:
mov si, ukloader_msg
call write_string
jmp loop

%include "BPB.inc"
%include "Macros.inc"
%include "Utility.inc"

; a little bit of fake padding
; to make sure we're able to load
; more than 1 sector
times 1000 db 0

ukloader_msg db "Hello world from UKLoader!", CR, LF, 0