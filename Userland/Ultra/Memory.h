#pragma once
#include <stddef.h>

void* virtual_alloc(void* address, size_t size);
void virtual_free(void* address, size_t size);
