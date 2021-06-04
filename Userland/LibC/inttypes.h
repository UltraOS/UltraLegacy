#pragma once

#include <stdint.h>

// ---- Normal ----
#define PRId8  "d"
#define PRId16 "d"
#define PRId32 "d"
#define PRId64 "lld"

#define PRIi8  "i"
#define PRIi16 "i"
#define PRIi32 "i"
#define PRIi64 "lli"

#define PRIo8  "o"
#define PRIo16 "o"
#define PRIo32 "o"
#define PRIo64 "lld"

#define PRIu8  "u"
#define PRIu16 "u"
#define PRIu32 "u"
#define PRIu64 "llu"

#define PRIx8  "x"
#define PRIx16 "x"
#define PRIx32 "x"
#define PRIx64 "llx"

#define PRIX8  "X"
#define PRIX16 "X"
#define PRIX32 "X"
#define PRIX64 "llX"

// ---- Least ----
#define PRIdLEAST8  "d"
#define PRIdLEAST16 "d"
#define PRIdLEAST32 "d"
#define PRIdLEAST64 "lld"

#define PRIiLEAST8  "i"
#define PRIiLEAST16 "i"
#define PRIiLEAST32 "i"
#define PRIiLEAST64 "lli"

#define PRIoLEAST8  "o"
#define PRIoLEAST16 "o"
#define PRIoLEAST32 "o"
#define PRIoLEAST64 "llo"

#define PRIuLEAST8  "u"
#define PRIuLEAST16 "u"
#define PRIuLEAST32 "u"
#define PRIuLEAST64 "llu"

#define PRIxLEAST8  "x"
#define PRIxLEAST16 "x"
#define PRIxLEAST32 "x"
#define PRIxLEAST64 "llx"

#define PRIXLEAST8  "X"
#define PRIXLEAST16 "X"
#define PRIXLEAST32 "X"
#define PRIXLEAST64 "llx"

#ifdef ULTRA_64
#define _PTR_PREFIX "l"
#else
#define _PTR_PREFIX
#endif

// ---- Fast ----
#define PRIdFAST8  _PTR_PREFIX "d"
#define PRIdFAST16 _PTR_PREFIX "d"
#define PRIdFAST32 _PTR_PREFIX "d"
#define PRIdFAST64 "lld"

#define PRIiFAST8  _PTR_PREFIX "i"
#define PRIiFAST16 _PTR_PREFIX "i"
#define PRIiFAST32 _PTR_PREFIX "i"
#define PRIiFAST64 "lli"

#define PRIoFAST8  _PTR_PREFIX "o"
#define PRIoFAST16 _PTR_PREFIX "o"
#define PRIoFAST32 _PTR_PREFIX "o"
#define PRIoFAST64 "llo"

#define PRIuFAST8  _PTR_PREFIX "u"
#define PRIuFAST16 _PTR_PREFIX "u"
#define PRIuFAST32 _PTR_PREFIX "u"
#define PRIuFAST64 "llu"

#define PRIxFAST8  _PTR_PREFIX "x"
#define PRIxFAST16 _PTR_PREFIX "x"
#define PRIxFAST32 _PTR_PREFIX "x"
#define PRIxFAST64 "llx"

#define PRIXFAST8  _PTR_PREFIX "X"
#define PRIXFAST16 _PTR_PREFIX "X"
#define PRIXFAST32 _PTR_PREFIX "X"
#define PRIXFAST64 "llx"

// Max
#define PRIdMAX "lld"
#define PRIiMAX "lli"
#define PRIoMAX "llo"
#define PRIuMAX "llu"
#define PRIxMAX "llx"
#define PRIXMAX "llX"

// Ptr
#define PRIdPTR _PTR_PREFIX "d"
#define PRIiPTR _PTR_PREFIX "i"
#define PRIoPTR _PTR_PREFIX "o"
#define PRIuPTR _PTR_PREFIX "u"
#define PRIxPTR _PTR_PREFIX "x"
#define PRIXPTR _PTR_PREFIX "X"