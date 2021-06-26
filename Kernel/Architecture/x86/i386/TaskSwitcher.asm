%define switch_task _ZN6kernel9Scheduler11switch_taskEPNS_6Thread12ControlBlockE
%define save_state  _ZN6kernel9Scheduler23save_state_and_scheduleEv
%define jump_to_userspace _ZN6kernel10TaskLoader17jump_to_userspaceEm

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
    mov eax, [esp]
    add esp, 4 ; don't leak the return eip

    pushf
    push cs
    push eax ; eip

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

global jump_to_userspace
jump_to_userspace:
    add esp, 4 ; sys-v abi, return eip -> arg0
    pop esp

    add esp, 4 ; skip ss
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 0x8 ; skip error code and interrupt number

    iret