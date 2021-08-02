#include "stdio.h"
#include "stdbool.h"
#include "limits.h"
#include "ctype.h"
#include "string.h"
#include "stdarg.h"
#include "stdlib.h"

#ifndef LIBC_TEST

#include <Ultra/Ultra.h>

typedef unsigned int wint_t;

#define DEFAULT_BUFFER_CAPACITY 512

static FILE* file_ptr_open(uint32_t io_handle, bool buffered)
{
    FILE* f = (FILE*)malloc(sizeof(FILE));
    f->io_handle = io_handle;

    if (buffered) {
        f->buffer = malloc(DEFAULT_BUFFER_CAPACITY);
        f->capacity = DEFAULT_BUFFER_CAPACITY;
    } else {
        f->capacity = 0;
    }

    f->size = 0;
}

static void file_ptr_close(FILE* f)
{
    free(f->buffer);
    free(f);
}

FILE* stdin = NULL;
FILE* stdout = NULL;
FILE* stderr = NULL;

void stdio_init()
{
    stdin = file_ptr_open(0, false);
    stdout = file_ptr_open(1, true);
    stderr = file_ptr_open(2, false);
}

int puts(const char* str)
{
    fwrite(str, 1, strlen(str), stdout);
    fwrite("\n", 1, 1, stdout);
    return 0;
}

int putchar(int character)
{
    fwrite(&character, 1, 1, stdout);
    return 0;
}

// Has to be under define, otherwise ADL gets us
int fprintf(FILE* stream, const char* format, ...)
{
    va_list list;
    va_start(list, format);
    int val = vfprintf(stream, format, list);
    va_end(list);

    return val;
}

int printf(const char* format, ...)
{
    va_list list;
    va_start(list, format);
    int val = vfprintf(stdout, format, list);
    va_end(list);

    return val;
}

int vprintf(const char* format, va_list vlist)
{
    return vfprintf(stdout, format, vlist);
}

FILE* fopen(const char* filename, const char* mode)
{
    long native_mode = 0;

    switch (mode[0])
    {
    case 'r':
        native_mode |= IO_READONLY;
        break;
    case 'w':
        native_mode |= IO_CREATE;
        native_mode |= IO_WRITEONLY;
        native_mode |= IO_TRUNCATE;
        break;
    case 'a':
        native_mode |= IO_CREATE;
        native_mode |= IO_APPEND;
        native_mode |= IO_WRITEONLY;
    default:
        return NULL;
    }

    bool fail_if_exists = false;

    for (size_t i = 0;; ++i) {
        switch (mode[i + 1])
        {
        case '+':
            if (native_mode & IO_READONLY)
                native_mode &= ~IO_READONLY;
            if (native_mode & IO_WRITEONLY)
                native_mode &= ~IO_WRITEONLY;
            if (native_mode & IO_APPEND)
                native_mode &= ~IO_WRITEONLY;
            native_mode |= IO_READWRITE;
            continue;
        case 'b':
            continue;
        case 'x':
            fail_if_exists = true;
            continue;
        default:
            break;
        }

        break;
    }

    // TODO: make a check
    (void) fail_if_exists;

    long io_handle = open_file(filename, native_mode);

    if (io_handle < 0)
        return NULL;

    FILE* ptr = file_ptr_open(io_handle, true);
    ptr->flags = native_mode;

    return ptr;
}

size_t fread(void* buffer, size_t size, size_t count, FILE* stream)
{
    if (!size || !count)
        return 0;

    if (stream->flags == IO_WRITEONLY)
        return 0;

    if (stream->flags & IO_READWRITE)
        fflush(stream);

    return io_read(stream->io_handle, buffer, size * count);
}

size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream)
{
    if (!size || !count)
        return 0;

    if (stream->flags == IO_READONLY)
        return 0;

    size_t bytes_to_write = size * count;
    size_t buf_free_bytes = stream->capacity - stream->size;

    if (bytes_to_write > buf_free_bytes)
        fflush(stream);

    if (bytes_to_write > stream->capacity)
        return io_write(stream->io_handle, buffer, bytes_to_write) / size;

    memcpy(stream->buffer + stream->size, buffer, bytes_to_write);
    stream->size += bytes_to_write;

    return count;
}

int fputc(int character, FILE* stream)
{
    char c = (char)character;
    return fwrite(&c, sizeof(c), 1, stream);
}

int fputs(const char* str, FILE* stream)
{
    size_t res = fwrite(str, sizeof(char), strlen(str), stream);
    return res ? res : EOF;
}

int fclose(FILE* stream)
{
    fflush(stream);
    io_close(stream->io_handle);
    file_ptr_close(stream);
}

int fflush(FILE* stream)
{
    if (!stream->size)
        return 0;

    io_write(stream->io_handle, stream->buffer, stream->size);
    stream->size = 0;
    return 0;
}

int fseek(FILE* stream, long offset, int origin)
{
    fflush(stream);
    long ret = io_seek(stream->io_handle, offset, origin);

    return ret < 0;
}

void rewind(FILE* stream)
{
    fflush(stream);
    fseek(stream, 0, SEEK_SET);
}

long ftell(FILE* stream)
{
    return io_seek(stream->io_handle, 0, SEEK_CUR);
}

int fscanf(FILE* stream, const char* format, ...)
{
    // TODO
    (void)stream;
    (void)format;

    return 0;
}

#define STACK_BUF_SIZE 4096

int vfprintf(FILE* stream, const char* format, va_list vlist)
{
    size_t bytes_needed = vsnprintf(NULL, 0, format, vlist);
    bytes_needed++; // '\0'

    if (bytes_needed > STACK_BUF_SIZE) {
        void* heap_buf = malloc(bytes_needed);
        vsnprintf(heap_buf, bytes_needed, format, vlist);
        fwrite(heap_buf, 1, bytes_needed - 1, stream);
        free(heap_buf);
        return 0;
    }

    char buffer[STACK_BUF_SIZE];
    vsnprintf(buffer, bytes_needed, format, vlist);
    fwrite(buffer, 1, bytes_needed - 1, stream);

    return 0;
}

int rename(const char* old_filename, const char* new_filename)
{
    return move_file(old_filename, new_filename);
}

int remove(const char* fname)
{
    return remove_file(fname);
}

#endif

int scanf(const char* format, ...)
{
    // TODO
    (void)format;

    return 0;
}

int sscanf(const char* buffer, const char* format, ...)
{
    // TODO:
    (void)buffer;
    (void)format;

    return 0;
}

int sprintf(char* buffer, const char* format, ...)
{
    va_list list;
    va_start(list, format);
    int val = vsnprintf(buffer, SIZE_MAX, format, list);
    va_end(list);

    return val;
}

int snprintf(char* buffer, size_t bufsz, const char* format, ...)
{
    va_list list;
    va_start(list, format);
    int val = vsnprintf(buffer, bufsz, format, list);
    va_end(list);

    return val;
}

int vsprintf(char* buffer, const char* format, va_list vlist)
{
    return vsnprintf(buffer, SIZE_MAX, format, vlist);
}

typedef struct {
    char* buffer;
    size_t bufsz;
    size_t offset;
} buffer_state;

static void write_one(buffer_state* state, char c)
{
    if (state->offset < (state->bufsz ? state->bufsz - 1 : 0))
        state->buffer[state->offset] = c;
    state->offset++;
}

enum justify_t {
    RIGHT,
    LEFT
};

typedef struct {
    int precision;
    int justify;
    int min_field_width;
    bool leading_zeroes;
    bool prepend_space;
    bool prepend_sign;
    bool alternative_form;
    bool has_precision;
    char conversion_specifier;
    char length_modifier[2];
} format_spec;

static void write_char(buffer_state* state, format_spec* spec, char c)
{
    write_one(state, c);
}

static void write_string(buffer_state* state, format_spec* spec, char* str, size_t stride)
{
    size_t max = spec->precision ? spec->precision : SIZE_MAX;

    for (size_t i = 0; *str && i < max; ++i) {
        write_one(state, *str);
        str += stride;
    }
}

static void write_number(buffer_state* state, format_spec* spec, char* number_repr, size_t repr_len, bool is_negative)
{
    if (spec->prepend_sign || is_negative)
        spec->prepend_space = false;
    if (spec->justify == LEFT || spec->has_precision)
        spec->leading_zeroes = false;

    if (spec->justify == LEFT) {
        size_t total_written = 0;

        if (spec->has_precision) {
            size_t zero_pad = repr_len >= spec->precision ? 0 : (spec->precision - repr_len);
            total_written += zero_pad;

            while (zero_pad--)
                write_one(state, '0');
        }

        if (is_negative) {
            write_one(state, '-');
            total_written++;
        } else if (spec->prepend_sign) {
            write_one(state, '+');
            total_written++;
        } else if (spec->prepend_space) {
            write_one(state, ' ');
            total_written++;
        }

        total_written += repr_len;

        size_t i = 0;
        while (repr_len--)
            write_one(state, number_repr[i++]);

        size_t additional_padding = (total_written > spec->min_field_width) ? 0 : (spec->min_field_width - total_written);
        while (additional_padding--)
            write_one(state, ' ');
    } else {
        size_t total_to_write = 0;
        size_t zero_pad = 0;

        if (spec->has_precision)
            zero_pad = (repr_len >= spec->precision) ? 0 : (spec->precision - repr_len);
        if (is_negative || spec->prepend_space || spec->prepend_sign)
            total_to_write++;

        size_t additional_padding = (total_to_write > spec->min_field_width) ? 0 : (spec->min_field_width - total_to_write);

        while (additional_padding--)
            write_one(state, spec->leading_zeroes ? '0' : ' ');

        if (is_negative)
            write_one(state, '-');
        else if (spec->prepend_sign)
            write_one(state, '+');
        else if (spec->prepend_space)
            write_one(state, ' ');

        while (zero_pad--)
            write_one(state, '0');

        size_t i = 0;
        while (repr_len--)
            write_one(state, number_repr[i++]);
    }
}

#define REPR_BUFFER_CAPACITY 64
static const char* characters = "0123456789ABCDEF";

static void write_signed_integral(buffer_state* state, format_spec* spec, long long base, int(*char_converter)(int), long long x)
{
    size_t offset = REPR_BUFFER_CAPACITY - 1;
    char num_buf[REPR_BUFFER_CAPACITY];

    if (x == 0) {
        char c = characters[x];
        num_buf[offset--] = char_converter ? (char)char_converter(c) : c;
    }

    bool is_negative = x < 0;

    while (x) {
        long long rem = x % base;

        if (rem < 0)
            rem *= -1;

        x /= base;

        char c = characters[rem];
        if (char_converter)
            c = (char)char_converter(c);

        num_buf[offset--] = c;
    }

    offset++;
    write_number(state, spec, num_buf + offset, REPR_BUFFER_CAPACITY - offset, is_negative);
}

static void write_unsigned_integral(buffer_state* state, format_spec* spec, unsigned long long base, int(*char_converter)(int), unsigned long long x)
{
    size_t offset = REPR_BUFFER_CAPACITY - 1;
    char num_buf[REPR_BUFFER_CAPACITY];

    if (x == 0) {
        char c = characters[x];
        num_buf[offset--] = char_converter ? (char)char_converter(c) : c;
    }

    while (x) {
        long long rem = x % base;
        x /= base;

        char c = characters[rem];
        if (char_converter)
            c = (char)char_converter(c);

        num_buf[offset--] = c;
    }

    offset++;
    write_number(state, spec, num_buf + offset, REPR_BUFFER_CAPACITY - offset, false);
}

static void write_int(buffer_state* state, format_spec* spec, long long x)
{
    if (spec->has_precision && spec->precision == 0 && x == 0)
        return;

    write_signed_integral(state, spec, 10, NULL, x);
}

static void write_hex(buffer_state* state, format_spec* spec, unsigned long long x)
{
    bool lowercase = spec->conversion_specifier == 'x';
    write_unsigned_integral(state, spec, 16, lowercase ? tolower : NULL, x);
}

static void write_unsigned(buffer_state* state, format_spec* spec, unsigned long long x)
{
    write_unsigned_integral(state, spec, 10, NULL, x);
}

static void write_octal(buffer_state* state, format_spec* spec, unsigned long long x)
{
    write_unsigned_integral(state, spec, 8, NULL, x);
}

static void write_float(buffer_state* state, format_spec* spec, long double x)
{
    // For now we ignore all conversions specifiers for float and write them as if
    // they were %f, TODO others later.
    // Also, this implementation is extremely naive. FIXME

    char num_buf[REPR_BUFFER_CAPACITY];
    size_t offset = REPR_BUFFER_CAPACITY - 1;

    bool is_negative = x < 0.0;
    if (is_negative)
        x = -x;

    unsigned long long lhs = (unsigned long long)x;

    int precision = 6;
    if (spec->has_precision)
        precision = spec->precision;

    // set to false because it means something else for numbers
    // so that write_number() works properly.
    spec->has_precision = false;

    if (precision || spec->alternative_form)
        num_buf[offset--] = '.';

    while (lhs) {
        num_buf[offset--] = (lhs % 10) + '0';
        lhs /= 10;
    }

    size_t bytes_written = REPR_BUFFER_CAPACITY - offset - 1;

    // short path
    if (precision == 0) {
        write_number(state, spec, num_buf + offset + 1, bytes_written, is_negative);
        return;
    }

    memmove(num_buf, num_buf + offset + 1, bytes_written);
    size_t bytes_left = REPR_BUFFER_CAPACITY - bytes_written;
    precision = bytes_left < precision ? bytes_left : precision;

    x -= lhs;

    for (; bytes_written < REPR_BUFFER_CAPACITY && precision--; bytes_written++) {
        x *= 10.;

        unsigned long long dec = (unsigned long long)x;

        num_buf[bytes_written] = '0' + dec % 10;
        x -= dec;
    }

    write_number(state, spec, num_buf, bytes_written, is_negative);
}

static bool dispatch_conversion(buffer_state* state, format_spec* spec, va_list* vlist)
{
    switch (spec->conversion_specifier) {
    case 'c': {
        if (spec->length_modifier[1])
            return false;
        if (spec->length_modifier[0]) {
            if (spec->length_modifier[0] != 'l')
                return false;

            wint_t arg = va_arg(*vlist, wint_t);
            write_char(state, spec, (unsigned char)arg);
        } else {
            int arg = va_arg(*vlist, int);
            write_char(state, spec, (unsigned char)arg);
        }

        break;
    }
    case 's': {
        if (spec->length_modifier[1])
            return false;
        if (spec->length_modifier[0]) {
            if (spec->length_modifier[0] != 'l')
                return false;

            wchar_t* arg = va_arg(*vlist, wchar_t*);
            write_string(state, spec, (char*)arg, sizeof(wchar_t));
        } else {
            char* arg = va_arg(*vlist, char*);
            write_string(state, spec, arg, sizeof(char));
        }

        break;
    }
    case 'd':
    case 'i':
        if (spec->length_modifier[0] == 'L')
            return false;

        switch (spec->length_modifier[0])
        {
        case 'h': {
            if (spec->length_modifier[1]) {
                if (spec->length_modifier[1] != 'h')
                    return false;

                signed char arg = (signed char)va_arg(*vlist, int);
                write_int(state, spec, arg);
            } else {
                short arg = (short)va_arg(*vlist, int);
                write_int(state, spec, arg);
            }

            break;
        }
        case '\0': {
            if (spec->length_modifier[1])
                return false;

            int arg = va_arg(*vlist, int);
            write_int(state, spec, arg);
            break;
        }
        case 'l': {
            if (spec->length_modifier[1]) {
                if (spec->length_modifier[1] != 'l')
                    return false;

                long long arg = va_arg(*vlist, long long);
                write_int(state, spec, arg);
            }
            else {
                long arg = va_arg(*vlist, long);
                write_int(state, spec, arg);
            }
            break;
        }
        case 'j': {
            if (spec->length_modifier[1])
                return false;

            intmax_t arg = va_arg(*vlist, intmax_t);
            write_int(state, spec, arg);
            break;
        }
        case 'z': {
            if (spec->length_modifier[1])
                return false;

            // relying on 2s complement
            size_t arg = va_arg(*vlist, size_t);
            write_int(state, spec, arg);
            break;
        }
        case 't': {
            if (spec->length_modifier[1])
                return false;

            ptrdiff_t arg = va_arg(*vlist, ptrdiff_t);
            write_int(state, spec, arg);
            break;
        }
        default:
            return false;
        }
        break;
    case 'o':
    case 'x':
    case 'X':
    case 'u': {
        void(*converter)(buffer_state*, format_spec*, unsigned long long) = NULL;

        switch (spec->conversion_specifier) {
        case 'o':
            converter = &write_octal;
            break;
        case 'x':
        case 'X':
            converter = &write_hex;
            break;
        case 'u':
            converter = &write_unsigned;
            break;
        }

        if (spec->length_modifier[0] == 'L')
            return false;

        switch (spec->length_modifier[0])
        {
        case 'h': {
            if (spec->length_modifier[1]) {
                if (spec->length_modifier[1] != 'h')
                    return false;

                unsigned char arg = (unsigned char)va_arg(*vlist, unsigned int);
                converter(state, spec, arg);
            } else {
                unsigned short arg = (unsigned short)va_arg(*vlist, unsigned int);
                converter(state, spec, arg);
            }

            break;
        }
        case '\0': {
            if (spec->length_modifier[1])
                return false;

            unsigned int arg = va_arg(*vlist, unsigned int);
            converter(state, spec, arg);
            break;
        }
        case 'l': {
            if (spec->length_modifier[1]) {
                if (spec->length_modifier[1] != 'l')
                    return false;

                unsigned long long arg = va_arg(*vlist, unsigned long long);
                converter(state, spec, arg);
            } else {
                unsigned long arg = va_arg(*vlist, unsigned long);
                converter(state, spec, arg);
            }
            break;
        }
        case 'j': {
            if (spec->length_modifier[1])
                return false;

            intmax_t arg = va_arg(*vlist, uintmax_t);
            converter(state, spec, arg);
            break;
        }
        case 'z': {
            if (spec->length_modifier[1])
                return false;

            size_t arg = va_arg(*vlist, size_t);
            converter(state, spec, arg);
            break;
        }
        case 't': {
            if (spec->length_modifier[1])
                return false;

            size_t arg = va_arg(*vlist, size_t);
            converter(state, spec, arg);
            break;
        }
        default:
            return false;
        }
        break;
    }
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'a':
    case 'A':
    case 'g':
    case 'G':
        if (spec->length_modifier[1])
            return false;

        switch (spec->length_modifier[0])
        {
        case '\0':
        case 'l': {
            double arg = va_arg(*vlist, double);
            write_float(state, spec, arg);
            break;
        }
        case 'L': {
            long double arg = va_arg(*vlist, long double);
            write_float(state, spec, arg);
            break;
        }
        }
        break;
    case 'n':
        if (spec->length_modifier[0] == 'L')
            return false;

        switch (spec->length_modifier[0])
        {
        case 'h': {
            if (spec->length_modifier[1]) {
                if (spec->length_modifier[1] != 'h')
                    return false;

                signed char* arg = va_arg(*vlist, signed char*);
                *arg = state->offset;
            } else {
                short* arg = va_arg(*vlist, short*);
                *arg = state->offset;
            }
            break;
        }
        case '\0': {
            if (spec->length_modifier[1])
                return false;

            int* arg = va_arg(*vlist, int*);
            *arg = state->offset;
            break;
        }
        case 'l': {
            if (spec->length_modifier[1]) {
                if (spec->length_modifier[1] != 'l')
                    return false;

                long long* arg = va_arg(*vlist, long long*);
                *arg = state->offset;
            } else {
                long* arg = va_arg(*vlist, long*);
                *arg = state->offset;
            }
            break;
        }
        case 'j': {
            if (spec->length_modifier[1])
                return false;

            intmax_t* arg = va_arg(*vlist, intmax_t*);
            *arg = state->offset;
            break;
        }
        case 'z': {
            if (spec->length_modifier[1])
                return false;

            size_t* arg = va_arg(*vlist, size_t*);
            *arg = state->offset;
            break;
        }
        case 't': {
            if (spec->length_modifier[1])
                return false;

            ptrdiff_t* arg = va_arg(*vlist, ptrdiff_t*);
            *arg = state->offset;
            break;
        }
        }
        break;
    case 'p':
        if (spec->length_modifier[0] || spec->length_modifier[1])
            return false;

        write_hex(state, spec, va_arg(*vlist, size_t));
        break;
    default:
        return false;
    }

    memset(spec, 0, sizeof(format_spec));
    return true;
}

static int consume_one(const char** str, bool* ok)
{
    if (**str == '\0') {
        *ok = false;
        return 0;
    }

    int token = **str;
    *ok = token != '\0';

    if (*ok)
        (*str)++;

    return token;
}

static int peek(const char** str, bool* ok)
{
    if (**str == '\0') {
        *ok = false;
        return 0;
    }

    int token = **str;
    *ok = token != '\0';

    return token;
}

static int parse_int(const char** str, bool* ok)
{
    size_t i = 0;
    bool did_consume = false;

    bool is_negative = false;
    char c = peek(str, &did_consume);

    if (!did_consume) {
        *ok = false;
        return 0;
    }

    if (c == '-' || c == '+') {
        is_negative = c == '-';
        consume_one(str, &did_consume);
    }

    int result = 0;
    bool consumed_at_least_one = false;

    for (;;) {
        c = peek(str, &did_consume);

        if (!did_consume || !isdigit(c)) {
            *ok = consumed_at_least_one;
            return result;
        }

        consume_one(str, &did_consume);
        consumed_at_least_one = true;

        if (result == 0 && c == '0')
            continue;

        char digit = c - '0';
        if (is_negative)
            digit *= -1;

        if (result > (INT_MAX / 10) || (result == (INT_MAX / 10) && digit > 7)) {
            *ok = false;
            return INT_MAX;
        }

        if (result < (INT_MIN / 10) || (result == (INT_MIN / 10) && digit < -8)) {
            *ok = false;
            return INT_MIN;
        }

        result *= 10;
        result += digit;
    }

    return result;
}

int vsnprintf(char* buffer, size_t bufsz, const char* format, va_list vlist_)
{
    enum {
        NORMAL,
        PERCENT,
        FIELD_WIDTH,
        PRECISION_DOT,
        PRECISION_NUMBER,
        LENGTH_MODIFIER,
        CONVERSION_SPECIFIER
    } state = NORMAL;

    buffer_state buf_state;
    buf_state.buffer = buffer;
    buf_state.bufsz = bufsz;
    buf_state.offset = 0;

    format_spec spec = { 0 };

    va_list vlist;
    va_copy(vlist, vlist_);

    for (;;) {
        bool ok = false;

        char c = consume_one(&format, &ok);
        if (!ok)
            break;

        switch (c)
        {
        case '%':
            if (state == NORMAL) {
                state = PERCENT;
                break;
            } else if (state == PERCENT) {
                write_one(&buf_state, c);
                state = NORMAL;
                memset(&spec, 0, sizeof(spec));
            } else {
                return -1;
            }
            break;
        case '-':
        case '+':
        case ' ':
        case '#':
            switch (state)
            {
            case NORMAL:
                write_one(&buf_state, c);
                break;
            case PRECISION_DOT:
            case PRECISION_NUMBER:
            case PERCENT:
                if (c == '-') {
                    spec.prepend_sign = true;
                    break;
                }

                if (c == '+') {
                    spec.justify = LEFT;
                    break;
                }

                state = PERCENT;

                if (c == ' ')
                    spec.prepend_space = true;
                if (c == '#')
                    spec.alternative_form = true;
                break;
            default:
                return -1;
            }
            if (state == NORMAL) {

               break;
            } else if (state == PERCENT || state == PRECISION_DOT || state == PRECISION_NUMBER) {
                spec.prepend_sign = true;
            } else {
                return -1;
            }
        case '*':
            if (state == PERCENT) {
                state = FIELD_WIDTH;
                int arg = va_arg(vlist, int);

                if (arg < 0) {
                    arg *= -1;
                    spec.justify = LEFT;
                }

                spec.min_field_width = arg;
            } else if (state == PRECISION_DOT) {
                state = PRECISION_NUMBER;
                int arg = va_arg(vlist, int);

                if (arg >= 0)
                    spec.precision = arg;
            } else if (state == NORMAL) {
                write_one(&buf_state, c);
            } else {
                return -1;
            }
        case '.':
            if (state == PERCENT || state == FIELD_WIDTH) {
                state = PRECISION_DOT;
                spec.has_precision = true;
                break;
            } else if (state == NORMAL) {
                write_one(&buf_state, c);
                break;
            }
            return -1;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (state == PERCENT) {
                if (c == '0') {
                    spec.leading_zeroes = true;
                    break;
                }

                state = FIELD_WIDTH;
                bool ok = false;
                --format;
                spec.min_field_width = parse_int(&format, &ok);
                if (!ok)
                    return -1;
                break;
            } else if (state == PRECISION_DOT) {
                state = PRECISION_NUMBER;
                bool ok = false;
                --format;
                spec.precision = parse_int(&format, &ok);
                if (!ok)
                    return -1;
                break;
            } else if (state == NORMAL) {
                write_one(&buf_state, c);
                break;
            }
            return -1;
        case 'c':
        case 's':
        case 'd':
        case 'i':
        case 'o':
        case 'x':
        case 'X':
        case 'u':
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'a':
        case 'A':
        case 'g':
        case 'G':
        case 'n':
        case 'p':
            if (state == NORMAL) {
                write_one(&buf_state, c);
                break;
            }

            spec.conversion_specifier = c;
            state = NORMAL;

            dispatch_conversion(&buf_state, &spec, &vlist);
            break;
        case 'h':
        case 'l':
        case 'j':
        case 'z':
        case 't':
        case 'L':
            if (state == NORMAL) {
                write_one(&buf_state, c);
                break;
            }

            // multiple length modifiers?
            if (state == LENGTH_MODIFIER)
                return -1;

            state = LENGTH_MODIFIER;
            spec.length_modifier[0] = c;

            switch (c)
            {
            case 'h': {
                bool ok = false;
                char token = peek(&format, &ok);

                if (token == 'h') {
                    spec.length_modifier[1] = token;
                    consume_one(&format, &ok);
                }
            }
            case 'l': {
                bool ok = false;
                char token = peek(&format, &ok);

                if (token == 'l') {
                    spec.length_modifier[1] = token;
                    consume_one(&format, &ok);
                }
            }
            }
            break;
        default: // NORMAL
            write_one(&buf_state, c);
        }
    }

    if (bufsz)
        buffer[buf_state.offset < bufsz ? buf_state.offset : bufsz] = '\0';

    return buf_state.offset;
}
