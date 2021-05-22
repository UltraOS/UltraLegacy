#pragma once

#include <stddef.h>

#if defined(__cplusplus) && !defined(LIBC_TEST)
extern "C" {
#endif

char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t count);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t count);
size_t strxfrm(char* dest, const char* src, size_t count);
// char* strdup(const char* str1); TODO
// char* strndup(const char* str1, size_t size); TODO

size_t strlen(const char* str);
int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, size_t count);
int strcoll(const char* lhs, const char* rhs);
char* strchr(const char* str, int ch);
char* strrchr(const char* str, int ch);
size_t strspn(const char* dest, const char* src);
size_t strcspn(const char* dest, const char* src);
char* strpbrk(const char* dest, const char* breakset);
char* strstr(const char* str, const char* substr);
// char* strtok(char* str, const char* delim); TODO

void* memchr(const void* ptr, int ch, size_t count);
int memcmp(const void* lhs, const void* rhs, size_t count);
void* memset(void* dest, int ch, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
void* memmove(void* dest, const void* src, size_t count);
void* memccpy(void* dest, const void* src, int ch, size_t count);

char* strerror(int errnum);

#if defined(__cplusplus) && !defined(LIBC_TEST)
}
#endif
