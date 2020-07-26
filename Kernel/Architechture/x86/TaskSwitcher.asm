%define switch_task _ZN6kernel9Scheduler11switch_taskEPNS_6Thread12ControlBlockES3_
%define user_data_selector 0x20
%define rpl_ring_3         0x3
%define current_task_ptr   esp + (4 * 5)
%define new_task_ptr       esp + (4 * 6)

section .text
; void Scheduler::switch_task(Thread::ControlBlock* current_task, Thread::ControlBlock* new_task)
global switch_task
switch_task:
    push ebx
    push esi
    push edi
    push ebp

    ; save the current esp
    mov esi, [current_task_ptr]
    mov [esi], esp ; current_task->current_kernel_stack_top = esp

    ; load the new task esp
    mov esi, [new_task_ptr]
    mov esp, [esi] ; esp = new_task->current_kernel_stack_top

    pop ebp
    pop edi
    pop esi
    pop ebx

    ret

global user_thread_entrypoint
user_thread_entrypoint:
    mov     ax, user_data_selector | rpl_ring_3
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    iret

global supervisor_thread_entrypoint
supervisor_thread_entrypoint:
    iret
