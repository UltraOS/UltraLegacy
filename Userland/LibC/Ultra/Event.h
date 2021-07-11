#pragma once

#include "VirtualKey.h"

enum ScrollDirection {
#define SCROLL_DIRECTION(direction) SC_ ## direction,
    ENUMERATE_SCROLL_DIRECTIONS
#undef SCROLL_DIRECTION
};

enum EventType {
#define EVENT_TYPE(type) ET_ ## type,
    ENUMERATE_EVENT_TYPES
#undef EVENT_TYPE
};

#include <Shared/Event.h>
