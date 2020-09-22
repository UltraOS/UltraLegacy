#pragma once

#define PACKED   [[gnu::packed]]
#define NOINLINE __attribute__((noinline))
#define ALWAYS_INLINE __attribute__((always_inline))

#define USED __attribute__((used))

#define SET_BIT(x) (1u << (x))

#define IS_BIT_SET(mask, x) ((mask)&SET_BIT(x))

#define TO_STRING(x) #x

#define cli() asm volatile("cli" ::: "memory")
#define sti() asm volatile("sti" ::: "memory")
#define hlt() asm volatile("hlt" ::: "memory")
#define hang()                                                                                                         \
    for (;;) {                                                                                                         \
        cli();                                                                                                         \
        hlt();                                                                                                         \
    }

#define pause __builtin_ia32_pause

#define KB (1024ull)
#define MB (1024ull * KB)
#define GB (1024ull * MB)

// A bunch of things to stop Visual Studio from
// complaining and actually help me instead
#ifdef _MSVC_LANG
#define cli()
#define sti()
#define hang()
#define pause()
#define asm()
#define USED
#define PACKED
#define ALWAYS_INLINE
#endif
