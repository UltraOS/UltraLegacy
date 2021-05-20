#pragma once

#include <stddef.h>

#if defined(__cplusplus) and !defined(LIBC_TEST)
extern "C" {
#endif

int bcmp(const void*, const void*, size_t);
void bcopy(const void*, void*, size_t);
void bzero(void*, size_t);
int ffs(int);
char* index(const char*, int);
char* rindex(const char*, int);
int strcasecmp(const char*, const char*);
int strncasecmp(const char*, const char*, size_t);

#if defined(__cplusplus) and !defined(LIBC_TEST)
}
#endif