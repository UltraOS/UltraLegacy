%define switch_task _ZN6kernel9Scheduler11switch_taskEPNS_6Thread12ControlBlockE
%define save_state  _ZN6kernel9Scheduler23save_state_and_scheduleEv
%define jump_to_userspace _ZN6kernel10TaskLoader17jump_to_userspaceEm

%define schedule    _ZN6kernel9Scheduler8scheduleEPKNS_13RegisterStateE
extern  schedule

%include "Common.inc"

section .text
; void Scheduler::switch_task(Thread::ControlBlock* new_task)
global switch_task
switch_task:
    ; load the new task rsp
    mov rsp, [rdi]

    popaq
    add rsp, 0x10
    iretq

global save_state
save_state:
    mov rsi, [rsp]
    add rsp, 8 ; don't leak the return RIP
    mov rax, rsp

    mov rdi, ss

    push rdi ; ss
    push rax ; rsp
    pushf

    mov  rax, cs
    push rax

    push rsi ; rip

    push qword 0
    push qword 0
    pushaq

    mov rdi, rsp
    cld
    call schedule

    ud2 ; in case we ever return

global jump_to_userspace
jump_to_userspace:
    mov rsp, rdi
    popaq
    add rsp, 0x10
    iretq
