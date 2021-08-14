#pragma once

#define ENUMERATE_SYSCALLS  \
    SYSCALL(EXIT_THREAD)    \
    SYSCALL(EXIT_PROCESS)   \
    SYSCALL(OPEN)           \
    SYSCALL(CLOSE)          \
    SYSCALL(READ)           \
    SYSCALL(WRITE)          \
    SYSCALL(SEEK)           \
    SYSCALL(TRUNCATE)       \
    SYSCALL(CREATE)         \
    SYSCALL(CREATE_DIR)     \
    SYSCALL(REMOVE)         \
    SYSCALL(REMOVE_DIR)     \
    SYSCALL(MOVE)           \
    SYSCALL(VIRTUAL_ALLOC)  \
    SYSCALL(VIRTUAL_FREE)   \
    SYSCALL(CREATE_THREAD)  \
    SYSCALL(CREATE_PROCESS) \
    SYSCALL(WM_COMMAND)     \
    SYSCALL(SLEEP)          \
    SYSCALL(TICKS)          \
    SYSCALL(DEBUG_LOG)      \
    SYSCALL(MAX)
