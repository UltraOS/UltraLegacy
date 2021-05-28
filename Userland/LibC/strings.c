#include "stdint.h"
#include "strings.h"
#include "string.h"
#include "ctype.h"

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
    size_t i = 0;

    while (lhs[i] && rhs[i]) {
        int lowerlhs = tolower(lhs[i]);
        int lowerrhs = tolower(rhs[i]);

        if (lowerlhs != lowerrhs)
            return (unsigned char)lowerlhs - (unsigned char)lowerrhs;

        i++;
    }

    return (unsigned char)tolower(lhs[i]) - (unsigned char)tolower(rhs[i]);
}

int strncasecmp(const char* lhs, const char* rhs, size_t count)
{
    if (count == 0)
        return 0;

    size_t i = 0;

    while (lhs[i] && rhs[i] && i < count) {
        int lowerlhs = tolower(lhs[i]);
        int lowerrhs = tolower(rhs[i]);

        if (lowerlhs != lowerrhs)
            return (unsigned char)lowerlhs - (unsigned char)lowerrhs;

        i++;
    }

    if (i == count)
        i--;

    return (unsigned char)tolower(lhs[i]) - (unsigned char)tolower(rhs[i]);
}
