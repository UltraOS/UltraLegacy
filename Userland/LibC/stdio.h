#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#if defined(__cplusplus) && !defined(LIBC_TEST)
extern "C" {
#endif

#ifndef LIBC_TEST
typedef struct {
    long io_handle;
    unsigned long flags;
    char* buffer;
    size_t size;
    size_t capacity;
} FILE;

// NOTE: must match Ultra/IO.h seek modes
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#define EOF -1
#endif

FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fflush(FILE* stream);
long ftell(FILE* stream);
int fseek(FILE* stream, long offset, int origin);
void rewind(FILE* stream);
size_t fread(void* buffer, size_t size, size_t count, FILE* stream);
size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
int fputc(int character, FILE* stream);
int fputs(const char* str, FILE* stream);

int scanf(const char* format, ... );
int fscanf(FILE* stream, const char* format, ...);
int sscanf(const char* buffer, const char* format, ...);

int puts(const char* str);
int putchar(int character);

int printf(const char* format, ...);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* buffer, const char* format, ...);
int snprintf(char* buffer, size_t bufsz, const char* format, ...);

int vprintf(const char* format, va_list vlist );
int vfprintf(FILE* stream, const char* format, va_list vlist);
int vsprintf(char* buffer, const char* format, va_list vlist);
int vsnprintf(char* buffer, size_t bufsz, const char* format, va_list vlist);

int rename(const char* old_filename, const char* new_filename);
int remove(const char* fname);

#if defined(__cplusplus) && !defined(LIBC_TEST)
}
#endif
