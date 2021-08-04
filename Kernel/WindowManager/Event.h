#pragma once

#include "VirtualKey.h"

namespace kernel {

enum class ScrollDirection : u8 {
#define SCROLL_DIRECTION(direction) direction,
    ENUMERATE_SCROLL_DIRECTIONS
#undef SCROLL_DIRECTION
};

enum class EventType : u8 {
#define EVENT_TYPE(type) type,
    ENUMERATE_EVENT_TYPES
#undef EVENT_TYPE
};

#define SCROLL_DIRECTION ScrollDirection
#define EVENT_TYPE EventType

#include "Shared/Event.h"

}
