%define safe_copy_memory _ZN6kernel16safe_copy_memoryEPKvPvm
%define length_of_user_string _ZN6kernel21length_of_user_stringEPKv
%define copy_until_null_or_n_from_user _ZN6kernel30copy_until_null_or_n_from_userEPKvPvm

; Safe function ABI:
; During a safe operation E(R)BX contains the address to go to in case of a fault,
; E(R)IP gets set to this address in case of a fault by the memory manager.

section .safe_operations

; Copies exactly bytes into dst, returns 1 if success or 0 if faulted.
; bool safe_copy_memory(void* src, void* dst, size_t bytes)
global safe_copy_memory
safe_copy_memory:
    push rbx

    ; swap src & dst for movsb
    xchg rdi, rsi

    ; third parameter to rcx
    mov rcx, rdx

    mov rbx, .on_access_violation

    rep movsb

    ; success, we didn't fault
    mov rax, 1
    pop rbx
    ret

    ; memory violation while copying :(
    .on_access_violation:
        mov rax, 0
        pop rbx
        ret

; returns string length including the null terminator, or 0 if faulted
; size_t length_of_user_string(void* str)
global length_of_user_string
length_of_user_string:
    ; preserve non-scratch
    push rbx

    mov rbx, .on_access_violation
    mov rax, 0 ; null terminator
    mov rcx, 0xFFFFFFFFFFFFFFFF

    repnz scasb

    mov rax, 0xFFFFFFFFFFFFFFFF
    sub rax, rcx

    pop rbx
    ret

    .on_access_violation:
        pop rbx
        mov rax, 0
        ret

; Copies up to max_length or up to the null byte to the dst from src.
; Returns the pointer to the end of dst or 0 if faulted.
; void* copy_until_null_or_n_from_user(void* src, void* dst, size_t max_length)
global copy_until_null_or_n_from_user
copy_until_null_or_n_from_user:
    ; preserve non-scratch
    push rbx

    mov rbx, .on_access_violation

    .next:
        ; if (length == 0) return
        or rdx, rdx
        jz .done

        ; char c = *src
        mov cl, byte [rdi]

        ; *dst = c
        mov [rsi], cl

        ; if (c == '\0') return
        or cl, cl
        jz .done

        ; src++, dst++, length--
        inc rdi
        inc rsi
        dec rdx

        jmp .next

    .done:
        pop rbx
        mov rax, rsi
        ret

    .on_access_violation:
        pop rbx
        mov rax, 0
        ret
