%define switch_task _ZN6kernel9Scheduler11switch_taskEPNS_6Thread12ControlBlockE
%define save_state  _ZN6kernel9Scheduler23save_state_and_scheduleEv

%define schedule    _ZN6kernel9Scheduler8scheduleEPKNS_13RegisterStateE
extern  schedule

%macro pushaq 0
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push rbp
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
%endmacro

%macro popaq 0
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rbp
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
%endmacro

section .text
; void Scheduler::switch_task(Thread::ControlBlock* new_task)
global switch_task
switch_task:
    ; load the new task esp
    mov rsp, [rdi]

    popaq
    add rsp, 0x10
    iretq

global save_state
save_state:
    mov rax, ss
    push rax

    push rsp
    pushf

    mov rax, cs
    push rax

    push qword [rsp + 8 * 4] ; eip

    push qword 0
    push qword 0

    pushaq
    mov rdi, rsp
    cld
    call schedule
    ud2
