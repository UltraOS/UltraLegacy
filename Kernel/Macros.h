#pragma once

#define PACKED [[gnu::packed]]

#define USED __attribute__((used))

#define SET_BIT(x) (1 << (x))

#define TO_STRING(x) #x

#define cli() asm volatile("cli" ::: "memory")
#define sti() asm volatile("sti" ::: "memory")