#pragma once

// signed types
using i8  = char;
using i16 = short int;
using i32 = int;
using i64 = long long int;

// unsigned types
using u8  = unsigned char;
using u16 = unsigned short int;
using u32 = unsigned int;
using u64 = unsigned long long int;

// MSVC complains about size_t redifinition
// let's just pretend we don't define it
// (this project cannot actually be compiled with MSVC anyway)
// I like using visual studio for editing tho.
#ifndef _MSVC_LANG
using size_t = long unsigned int;
#endif

using ptr_t  = u32;

// floating point
using f32 = float;
using f64 = double;

#define SIZEOF_I8  1
#define SIZEOF_I16 2
#define SIZEOF_I32 4
#define SIZEOF_I64 8

#define SIZEOF_U8  1
#define SIZEOF_U16 2
#define SIZEOF_U32 4
#define SIZEOF_U64 8

#define SIZEOF_F32 4
#define SIZEOF_F64 8

static_assert(sizeof(i8)  == SIZEOF_I8,  "Incorrect size of 8 bit integer");
static_assert(sizeof(i16) == SIZEOF_I16, "Incorrect size of 16 bit integer");
static_assert(sizeof(i32) == SIZEOF_I32, "Incorrect size of 32 bit integer");
static_assert(sizeof(i64) == SIZEOF_I64, "Incorrect size of 64 bit integer");

static_assert(sizeof(u8)  == SIZEOF_U8,  "Incorrect size of 8 bit unsigned integer");
static_assert(sizeof(u16) == SIZEOF_U16, "Incorrect size of 16 bit unsigned integer");
static_assert(sizeof(u32) == SIZEOF_U32, "Incorrect size of 32 bit unsigned integer");
static_assert(sizeof(u64) == SIZEOF_U64, "Incorrect size of 64 bit unsigned integer");

static_assert(sizeof(f32) == SIZEOF_F32, "Incorrect size of 32 bit float");
static_assert(sizeof(f64) == SIZEOF_F64, "Incorrect size of 64 bit float");

#undef SIZEOF_F32
#undef SIZEOF_F64

#undef SIZEOF_U8
#undef SIZEOF_U16
#undef SIZEOF_U32
#undef SIZEOF_U64

#undef SIZEOF_I8
#undef SIZEOF_I16
#undef SIZEOF_I32
#undef SIZEOF_I64
