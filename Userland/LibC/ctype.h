#pragma once

#include <stdint.h>

#if defined(__cplusplus) && !defined(LIBC_TEST)
extern "C" {
#endif

#define _CTYPE_CONTROL (1 << 0)
#define _CTYPE_SPACE   (1 << 1)
#define _CTYPE_BLANK   (1 << 2)
#define _CTYPE_PUNCT   (1 << 3)
#define _CTYPE_LOWER   (1 << 4)
#define _CTYPE_UPPER   (1 << 5)
#define _CTYPE_DIGIT   (1 << 6)
#define _CTYPE_XDIGIT  (1 << 7)
#define _CTYPE_ALPHA   (_CTYPE_LOWER | _CTYPE_UPPER)
#define _CTYPE_ALNUM   (_CTYPE_ALPHA | _CTYPE_DIGIT)

extern const uint8_t __ctype_table[256];

int isalnum(int ch);
int isalpha(int ch);
int islower(int ch);
int isupper(int ch);
int isdigit(int ch);
int isxdigit(int ch);
int iscntrl(int ch);
int isgraph(int ch);
int isspace(int ch);
int isblank(int ch);
int isprint(int ch);
int ispunct(int ch);

int tolower(int ch);
int toupper(int ch);

inline int __fast_isalnum(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_ALNUM; }
inline int __fast_isalpha(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_ALPHA; }
inline int __fast_islower(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_LOWER; }
inline int __fast_isupper(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_UPPER; }
inline int __fast_isdigit(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_DIGIT; }
inline int __fast_isxdigit(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_XDIGIT; }
inline int __fast_iscntrl(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_CONTROL; }
inline int __fast_isgraph(int ch) { return (__ctype_table[(unsigned char)ch] & (_CTYPE_CONTROL | _CTYPE_SPACE)) == 0; }
inline int __fast_isspace(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_SPACE; }
inline int __fast_isblank(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_BLANK; }
inline int __fast_isprint(int ch) { return (__ctype_table[(unsigned char)ch] & _CTYPE_CONTROL) == 0; }
inline int __fast_ispunct(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_PUNCT; }

inline int __fast_tolower(int ch)
{
    if (__fast_isupper(ch))
        return ch + ('a' - 'A');

    return ch;
}

inline int __fast_toupper(int ch)
{
    if (__fast_islower(ch))
        return ch - ('a' - 'A');

    return ch;
}

#define isalnum __fast_isalnum 
#define isalpha __fast_isalpha
#define islower __fast_islower
#define isupper __fast_isupper
#define isdigit __fast_isdigit
#define isxdigit __fast_isxdigit
#define iscntrl __fast_iscntrl
#define isgraph __fast_isgraph
#define isspace __fast_isspace
#define isblank __fast_isblank
#define isprint __fast_isprint
#define ispunct __fast_ispunct

#define tolower __fast_tolower
#define toupper __fast_toupper

#if defined(__cplusplus) && !defined(LIBC_TEST)
}
#endif
