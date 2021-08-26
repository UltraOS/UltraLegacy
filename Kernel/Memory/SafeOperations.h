#pragma once

#include "Common/Macros.h"

namespace kernel {

// -------------------------------------------------
// implemented in Architecture/X/SafeOperations.asm
// -------------------------------------------------

// Copies exactly 'bytes' from source into destination, returns true if success or false if faulted.
NOINLINE bool safe_copy_memory(const void* source, void* destination, size_t bytes);

// Returns string length including the null terminator, or 0 if faulted.
NOINLINE size_t length_of_user_string(const void* string);

// Copies up to max_length or up to the null byte to the dst from src.
// Returns the number of bytes copied including the null byte, or 0 if faulted.
NOINLINE size_t copy_until_null_or_n_from_user(const void* source, void* destination, size_t max_length);

}
