#include "stdint.h"
#include "strings.h"
#include "string.h"

int bcmp(const void* lhs, const void* rhs, size_t size)
{
    const uint8_t* byte_lhs = (const uint8_t*)lhs;
    const uint8_t* byte_rhs = (const uint8_t*)rhs;

    while (size--) {
        if (*byte_lhs++ != *byte_rhs++)
            return 1;
    }

    return 0;
}

void bcopy(const void* src, void* dest, size_t count)
{
    memmove(dest, src, count);
}

void bzero(void* dest, size_t count)
{
    memset(dest, 0, count);
}

int ffs(int number)
{
#ifdef __GNUC__
    return __builtin_ffs(number);
#elif defined(_MSC_VER)
    return 0;
#else
    static_assert(false, "?");
#endif
}

char* index(const char* src, int number)
{
    return strchr(src, number);
}

char* rindex(const char* src, int number)
{
    return strrchr(src, number);
}

int strcasecmp(const char* lhs, const char* rhs)
{
    return 0;
}

int strncasecmp(const char* lhs, const char* rhs, size_t count)
{
    return 1;
}
