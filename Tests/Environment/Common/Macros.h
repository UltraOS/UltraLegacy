#pragma once

#define hang()

#define KB (1024ull)
#define MB (1024ull * KB)
#define GB (1024ull * MB)

#define SET_BIT(x) (1ull << (x))

#define IS_BIT_SET(mask, x) ((mask)&SET_BIT(x))

// Koenig lookup workaround
#include "Utilities.h"
#define move    ::kernel::move
#define forward ::kernel::forward

#define MAKE_NONCOPYABLE(class_name)        \
    class_name(const class_name&) = delete; \
    class_name& operator=(const class_name&) = delete;

#define MAKE_NONMOVABLE(class_name)    \
    class_name(class_name&&) = delete; \
    class_name& operator=(class_name&&) = delete;

#define MAKE_STATIC(class_name)  \
    MAKE_NONCOPYABLE(class_name) \
    MAKE_NONMOVABLE(class_name)  \
    class_name() = delete;

#define MAKE_SINGLETON(class_name, ...) \
    MAKE_NONCOPYABLE(class_name)        \
    MAKE_NONMOVABLE(class_name)         \
private:                                \
    class_name(__VA_ARGS__)

#define PACKED
