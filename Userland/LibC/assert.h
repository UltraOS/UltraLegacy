#pragma once

#ifdef NDEBUG
#define assert(ignore)((void) 0)
#else
void __on_failed_assertion(const char* text_expression, const char* file, const char* function, size_t line);
#define assert(expression) (expression) ? (void)(0) : __on_failed_assertion(#expression, __FILE__, __func__, __LINE__)
#endif
