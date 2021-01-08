%define switch_task _ZN6kernel9Scheduler11switch_taskEPNS_6Thread12ControlBlockE
%define save_state  _ZN6kernel9Scheduler23save_state_and_scheduleEv

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
    mov rax, rsp
    mov rdi, ss

    push rdi ; rsp
    push rax ; ss
    pushf

    mov  rax, cs
    push rax

    push qword [rsp + 8 * 4] ; rip

    push qword 0
    push qword 0
    pushaq

    mov rdi, rsp
    cld
    call schedule

    ud2 ; in case we ever return
