#pragma once

#define ASSERT(x)
#define hang()

#define KB (1024ull)
#define MB (1024ull * KB)
#define GB (1024ull * MB)

// Koenig lookup workaround
#include "Utilities.h"
#define move    ::kernel::move
#define forward ::kernel::forward
