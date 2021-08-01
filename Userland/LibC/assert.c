#include "stdlib.h"
#include "stdio.h"

void __on_failed_assertion(const char* text_expression, const char* file, const char* function, size_t line)
{
    fprintf(stderr, "Assertion \"%s\" failed at %s:%zu (%s)", text_expression, file, line, function);
    abort();
}
