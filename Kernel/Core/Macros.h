#pragma once

#define PACKED [[gnu::packed]]

#define USED __attribute__((used))

#define SET_BIT(x) (1 << (x))

#define IS_BIT_SET(mask, x) ((mask) & SET_BIT(x))

#define TO_STRING(x) #x

#define cli()  asm volatile("cli" ::: "memory")
#define sti()  asm volatile("sti" ::: "memory")
#define hlt()  asm volatile("hlt" ::: "memory")
#define hang() for (;;) { cli(); hlt(); }

#define KB (1024)
#define MB (1024 * KB)
#define GB (1024 * MB)
