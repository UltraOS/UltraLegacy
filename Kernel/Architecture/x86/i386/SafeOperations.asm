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
    ; preserve non-scratch registers
    push esi
    push edi
    push ebx

    mov esi, [esp + 16]  ; src
    mov edi, [esp + 20]  ; dst
    mov ecx, [esp + 24]  ; bytes
    mov ebx, .on_access_violation ; label to jump to in case of a fault

    rep movsb

    mov eax, 1
    jmp .done

    .on_access_violation:
        xor eax, eax
    .done:
        pop ebx
        pop edi
        pop esi

        ret

; Returns string length including the null terminator, or 0 if faulted
; size_t length_of_user_string(void* str)
global length_of_user_string
length_of_user_string:
    ; preserve non-scratch
    push ecx
    push edi
    push ebx

    mov ebx, .on_access_violation
    mov eax, 0 ; null terminator
    mov ecx, 0xFFFFFFFF
    mov edi, [esp + 16]

    repnz scasb

    mov eax, 0xFFFFFFFF
    sub eax, ecx
    jmp .done

    .on_access_violation:
        xor eax, eax
    .done:
        pop ebx
        pop edi
        pop ecx

        ret

; Copies up to max_length or up to the null byte to the dst from src.
; Returns the number of bytes copied including the null byte, or 0 if faulted.
; size_t copy_until_null_or_n_from_user(void* src, void* dst, size_t max_length)
global copy_until_null_or_n_from_user
copy_until_null_or_n_from_user:
    ; preserve non-scratch
    push ebx
    push esi
    push edi

    mov ebx, .on_access_violation
    xor eax, eax
    mov edi, [esp + 16]
    mov esi, [esp + 20]
    mov ecx, [esp + 24]

    .next:
        ; if (length == 0) return
        or ecx, ecx
        jz .done

        ; char c = *src
        mov dl, byte [edi]

        ; *dst = c
        mov [esi], dl

        ; bytes_copied++
        inc eax

        ; if (c == \0) return
        or dl, dl
        jz .done

        ; src++, dst++, length--
        inc edi
        inc esi
        dec ecx

        jmp .next

    .on_access_violation:
        xor eax, eax
    .done:
        pop edi
        pop esi
        pop ebx
        ret
