%define switch_task _ZN6kernel9Scheduler11switch_taskEPNS_6Thread12ControlBlockES3_

section .text
; void Scheduler::switch_task(Thread::ControlBlock* current_task, Thread::ControlBlock* new_task)
global switch_task
switch_task:
    push ebx
    push esi
    push edi
    push ebp
    xchg bx, bx
    ; save the current esp
    mov esi, [esp + (4 * 5)]
    mov [esi], esp ; ControlBlock.kernel_stack_top = esp

    ; load the new task esp
    mov esi, [esp + (4 * 6)]
    mov esp, [esi] ; esp = ControlBlock.kernel_stack_top

    pop ebp
    pop edi
    pop esi
    pop ebx

    sti

    ret