#define ASSERT(x)
#define hang()

// Koenig lookup workaround
#include "Utilities.h"
#define move    ::kernel::move
#define forward ::kernel::forward