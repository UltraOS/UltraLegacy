#include "string.h"
#include "stdint.h"

char* strcpy(char* dest, const char* src)
{
    char* org_dest = dest;

    do {
        *dest++ = *src++;
    } while (*src);

    return org_dest;
}

char* strncpy(char* dest, const char* src, size_t count)
{
    size_t i = 0;
    for (; i < count && *src; ++i)
        dest[i] = *src++;

    for (; i < count; ++i)
        dest[i] = '\0';

    return dest;
}

char* strcat(char* dest, const char* src)
{
    size_t offset = strlen(dest);

    size_t i  = 0;
    for (; src[i]; ++i)
        dest[offset + i] = src[i];
    dest[offset + i] = '\0';

    return dest;
}

char* strncat(char* dest, const char* src, size_t count)
{
    size_t offset = strlen(dest);

    size_t i = 0;
    for (; src[offset + i] && i < count; ++i)
        dest[i] = src[i];
    dest[offset + i] = '\0';

    return dest;
}

size_t strxfrm(char* dest, const char* src, size_t count)
{
    size_t i = 0;
    for (; i < count && *src; ++i)
        dest[i] = *src++;

    for (; i < count; ++i)
        dest[i] = '\0';

    return i;
}

size_t strlen(const char* str)
{
    size_t len = 0;

    while (*str++)
        len++;

    return len;
}

int strcmp(const char* lhs, const char* rhs)
{
    typedef const unsigned char* cucp;

    size_t i = 0;

    while (lhs[i] && rhs[i]) {
        if (lhs[i] != rhs[i])
            return *(cucp)&lhs[i] - *(cucp)&rhs[i];

        i++;
    }

    return *(cucp)&lhs[i] - *(cucp)&rhs[i];
}

int strncmp(const char* lhs, const char* rhs, size_t count)
{
    typedef const unsigned char* cucp;

    if (count == 0)
        return 0;

    size_t i = 0;

    while (lhs[i] && rhs[i] && i < count) {
        if (lhs[i] != rhs[i])
            return *(cucp)&lhs[i] - *(cucp)&rhs[i];

        i++;
    }

    return *(cucp)&lhs[i] - *(cucp)&rhs[i];
}

int strcoll( const char* lhs, const char* rhs)
{
    return strcmp(lhs, rhs);
}

char* strchr(const char* str, int ch)
{
    char c = ch;

    for (;; str++) {
        if (*str == c)
            return (char*)str;
        if (!*str)
            return NULL;
    }
}

char* strrchr(const char* str, int ch)
{
    char c = ch;
    char* pos = NULL;

    for (;; str++) {
        if (*str == c)
            pos = (char*)str;
        if (!*str)
            return pos;
    }
}

size_t string_span(const char* dest, const char* src, uint8_t count_if, char** out_ptr)
{
    uint8_t table[256];
    memset(table, 0, 256);

    for (; *src; src++)
        table[(unsigned int)*src] = 1;

    size_t count = 0;
    for (; *dest; dest++, count++) {
        if (table[*(unsigned int*)dest] == count_if)
            continue;

        if (out_ptr)
            *out_ptr = (char*)dest;
        break;
    }

    return count;
}

size_t strspn(const char* dest, const char* src)
{
    return string_span(dest, src, 1, NULL);
}

size_t strcspn(const char* dest, const char* src)
{
    return string_span(dest, src, 0, NULL);
}

char* strpbrk(const char* dest, const char* breakset)
{
    char* result = NULL;
    string_span(dest, breakset, 0, &result);
    return result;
}

char* strstr(const char* str, const char* substr)
{
    size_t src_len = strlen(str);
    size_t substr_len = strlen(substr);

    if (substr_len > src_len)
        return NULL;
    if (substr_len == 0)
        return (char*)str;

    for (size_t i = 0; i < src_len - substr_len + 1; ++i) {
        if (str[i] != substr[0])
            continue;

        size_t j = i;
        size_t k = 0;

        while (k < substr_len) {
            if (str[j++] != substr[k])
                break;

            k++;
        }

        if (k == substr_len)
            return (char*)str;
    }
}

void* memchr(const void* ptr, int ch, size_t count)
{
    const uint8_t* byte_ptr = (uint8_t*)ptr;
    uint8_t byte = (uint8_t)ch;

    for (size_t i = 0; i < count; ++i) {
        if (byte_ptr[i] == byte)
            return (void*)byte_ptr;
    }

    return NULL;
}

void* memset(void* dest, int ch, size_t count)
{
    uint8_t fill = (uint8_t)ch;

    uint8_t* byte_dest = (uint8_t*)dest;
    for (size_t i = 0; i < count; ++i)
        byte_dest[i] = fill;

    return dest;
}

int memcmp(const void* lhs, const void* rhs, size_t count)
{
    const uint8_t* byte_lhs = (const uint8_t*)lhs;
    const uint8_t* byte_rhs = (const uint8_t*)rhs;

    size_t i = 0;

    for (; i < count; ++i) {
        if (byte_lhs[i] != byte_rhs[i])
            return byte_lhs[i] - byte_rhs[i];
    }

    return 0;
}

void* memcpy(void* dest, const void* src, size_t count)
{
    uint8_t* byte_dest = (uint8_t*)dest;
    const uint8_t* byte_src = (const uint8_t*)src;
    for (size_t i = 0; i < count; ++i)
        byte_dest[i] = byte_src[i];

    return dest;
}

void* memmove(void* dest, const void* src, size_t count)
{
    uint8_t* byte_dest = (uint8_t*)dest;
    const uint8_t* byte_src = (const uint8_t*)src;

    if (src < dest) {
        byte_src += count;
        byte_dest += count;

        while (count--)
            *--byte_dest = *--byte_src;
    } else {
        memcpy(dest, src, count);
    }

    return dest;
}

void* memccpy(void* dest, const void* src, int ch, size_t count)
{
    uint8_t* byte_dest = (uint8_t*)dest;
    const uint8_t* byte_src = (const uint8_t*)src;
    unsigned char c = (unsigned char)ch;

    for (size_t i = 0; i < count; ++i) {
        uint8_t val = byte_src[i];
        byte_dest[i] = val;

        if (val == c)
            return byte_dest + 1;
    }

    return NULL;
}

char* strerror(int errnum)
{
    return (char*)"UNIMPLEMENTED";
}
