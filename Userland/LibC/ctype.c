#include "ctype.h"

const uint8_t __ctype_table[256] = {
   _CTYPE_CONTROL, // 0
   _CTYPE_CONTROL, // 1
   _CTYPE_CONTROL, // 2
   _CTYPE_CONTROL, // 3
   _CTYPE_CONTROL, // 4
   _CTYPE_CONTROL, // 5
   _CTYPE_CONTROL, // 6
   _CTYPE_CONTROL, // 7
   _CTYPE_CONTROL, // -> 8 control codes

   _CTYPE_CONTROL | _CTYPE_SPACE | _CTYPE_BLANK, // 9 tab

   _CTYPE_CONTROL | _CTYPE_SPACE, // 10
   _CTYPE_CONTROL | _CTYPE_SPACE, // 11
   _CTYPE_CONTROL | _CTYPE_SPACE, // 12
   _CTYPE_CONTROL | _CTYPE_SPACE, // -> 13 whitespaces

   _CTYPE_CONTROL, // 14
   _CTYPE_CONTROL, // 15
   _CTYPE_CONTROL, // 16
   _CTYPE_CONTROL, // 17
   _CTYPE_CONTROL, // 18
   _CTYPE_CONTROL, // 19
   _CTYPE_CONTROL, // 20
   _CTYPE_CONTROL, // 21
   _CTYPE_CONTROL, // 22
   _CTYPE_CONTROL, // 23
   _CTYPE_CONTROL, // 24
   _CTYPE_CONTROL, // 25
   _CTYPE_CONTROL, // 26
   _CTYPE_CONTROL, // 27
   _CTYPE_CONTROL, // 28
   _CTYPE_CONTROL, // 29
   _CTYPE_CONTROL, // 30
   _CTYPE_CONTROL, // -> 31 control codes

   _CTYPE_SPACE | _CTYPE_BLANK, // 32 space

   _CTYPE_PUNCT, // 33
   _CTYPE_PUNCT, // 34
   _CTYPE_PUNCT, // 35
   _CTYPE_PUNCT, // 36
   _CTYPE_PUNCT, // 37
   _CTYPE_PUNCT, // 38
   _CTYPE_PUNCT, // 39
   _CTYPE_PUNCT, // 40
   _CTYPE_PUNCT, // 41
   _CTYPE_PUNCT, // 42
   _CTYPE_PUNCT, // 43
   _CTYPE_PUNCT, // 44
   _CTYPE_PUNCT, // 45
   _CTYPE_PUNCT, // 46
   _CTYPE_PUNCT, // -> 47 punctuation

   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 48
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 49
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 50
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 51
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 52
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 53
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 54
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 55
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // 56
   _CTYPE_DIGIT | _CTYPE_XDIGIT, // -> 57 digits

   _CTYPE_PUNCT, // 58
   _CTYPE_PUNCT, // 59
   _CTYPE_PUNCT, // 60
   _CTYPE_PUNCT, // 61
   _CTYPE_PUNCT, // 62
   _CTYPE_PUNCT, // 63
   _CTYPE_PUNCT, // -> 64 punctuation

   _CTYPE_UPPER | _CTYPE_XDIGIT, // 65
   _CTYPE_UPPER | _CTYPE_XDIGIT, // 66
   _CTYPE_UPPER | _CTYPE_XDIGIT, // 67
   _CTYPE_UPPER | _CTYPE_XDIGIT, // 68
   _CTYPE_UPPER | _CTYPE_XDIGIT, // 69
   _CTYPE_UPPER | _CTYPE_XDIGIT, // -> 70 ABCDEF

   _CTYPE_UPPER, // 71
   _CTYPE_UPPER, // 72
   _CTYPE_UPPER, // 73
   _CTYPE_UPPER, // 74
   _CTYPE_UPPER, // 75
   _CTYPE_UPPER, // 76
   _CTYPE_UPPER, // 77
   _CTYPE_UPPER, // 78
   _CTYPE_UPPER, // 79
   _CTYPE_UPPER, // 80
   _CTYPE_UPPER, // 81
   _CTYPE_UPPER, // 82
   _CTYPE_UPPER, // 83
   _CTYPE_UPPER, // 84
   _CTYPE_UPPER, // 85
   _CTYPE_UPPER, // 86
   _CTYPE_UPPER, // 87
   _CTYPE_UPPER, // 88
   _CTYPE_UPPER, // 89
   _CTYPE_UPPER, // -> 90 the rest of UPPERCASE alphabet

   _CTYPE_PUNCT, // 91
   _CTYPE_PUNCT, // 92
   _CTYPE_PUNCT, // 93
   _CTYPE_PUNCT, // 94
   _CTYPE_PUNCT, // 95
   _CTYPE_PUNCT, // -> 96 punctuation

   _CTYPE_LOWER | _CTYPE_XDIGIT, // 97
   _CTYPE_LOWER | _CTYPE_XDIGIT, // 98
   _CTYPE_LOWER | _CTYPE_XDIGIT, // 99
   _CTYPE_LOWER | _CTYPE_XDIGIT, // 100
   _CTYPE_LOWER | _CTYPE_XDIGIT, // 101
   _CTYPE_LOWER | _CTYPE_XDIGIT, // -> 102 abcdef

   _CTYPE_LOWER, // 103
   _CTYPE_LOWER, // 104
   _CTYPE_LOWER, // 105
   _CTYPE_LOWER, // 106
   _CTYPE_LOWER, // 107
   _CTYPE_LOWER, // 108
   _CTYPE_LOWER, // 109
   _CTYPE_LOWER, // 110
   _CTYPE_LOWER, // 111
   _CTYPE_LOWER, // 112
   _CTYPE_LOWER, // 113
   _CTYPE_LOWER, // 114
   _CTYPE_LOWER, // 115
   _CTYPE_LOWER, // 116
   _CTYPE_LOWER, // 117
   _CTYPE_LOWER, // 118
   _CTYPE_LOWER, // 119
   _CTYPE_LOWER, // 120
   _CTYPE_LOWER, // 121
   _CTYPE_LOWER, // -> 122 the rest of UPPERCASE alphabet

   _CTYPE_PUNCT, // 123
   _CTYPE_PUNCT, // 124
   _CTYPE_PUNCT, // 125
   _CTYPE_PUNCT, // -> 126 punctuation

   _CTYPE_CONTROL // 127 backspace
};

#undef isalnum 
#undef isalpha
#undef islower
#undef isupper
#undef isdigit
#undef isxdigit
#undef iscntrl
#undef isgraph
#undef isspace
#undef isblank
#undef isprint
#undef ispunct
#undef tolower
#undef toupper

int isalnum(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_ALNUM; }
int isalpha(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_ALPHA; }
int islower(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_LOWER; }
int isupper(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_UPPER; }
int isdigit(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_DIGIT; }
int isxdigit(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_XDIGIT; }
int iscntrl(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_CONTROL; }
int isgraph(int ch) { return (__ctype_table[(unsigned char)ch] & (_CTYPE_CONTROL | _CTYPE_SPACE)) == 0; }
int isspace(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_SPACE; }
int isblank(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_BLANK; }
int isprint(int ch) { return  (__ctype_table[(unsigned char)ch] & _CTYPE_CONTROL) == 0; }
int ispunct(int ch) { return __ctype_table[(unsigned char)ch] & _CTYPE_PUNCT; }

inline int tolower(int ch)
{
    if (isupper(ch))
        return ch + ('a' - 'A');

    return ch;
}

inline int toupper(int ch)
{
    if (islower(ch))
        return ch - ('a' - 'A');

    return ch;
}
