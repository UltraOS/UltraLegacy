#pragma once

#include <stddef.h>

int abs(int n);
long labs(long n);
long long llabs(long long n);

// vvvv implemented in malloc.c vvvv
void* calloc(size_t num, size_t size );
void* malloc(size_t size);
void* realloc(void* ptr, size_t new_size);
void free(void* ptr);

void exit(int exit_code);

int atoi(const char* str);
double atof(const char* str);

void abort();
