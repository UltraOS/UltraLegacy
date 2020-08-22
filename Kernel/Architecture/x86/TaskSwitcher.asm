%define switch_task _ZN6kernel9Scheduler11switch_taskEPNS_6Thread12ControlBlockE
%define save_state  _ZN6kernel9Scheduler23save_state_and_scheduleEv

%define schedule    _ZN6kernel9Scheduler8scheduleEPKNS_13RegisterStateE
extern  schedule

section .text
; void Scheduler::switch_task(Thread::ControlBlock* new_task)
global switch_task
switch_task:
    ; load the new task esp
    mov esi, [esp + 4]
    mov esp, [esi]

    add esp, 4 ; skip ss
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 0x8 ; skip error code and interrupt number
    iret

global save_state
save_state:
    pushf
    push cs
    push dword [esp + 8] ; eip

    push dword 0
    push dword 0
    pusha
    push ds
    push es
    push fs
    push gs
    push ss
    push esp
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    cld
    call schedule
    ud2
