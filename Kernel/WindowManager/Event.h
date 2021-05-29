#pragma once

#include "VirtualKey.h"

enum class ScrollDirection : uint8_t {
#define SCROLL_DIRECTION(direction) direction,
    ENUMERATE_SCROLL_DIRECTIONS
#undef SCROLL_DIRECTION
};

enum class EventType : uint8_t {
#define EVENT_TYPE(type) type,
    ENUMERATE_EVENT_TYPES
#undef EVENT_TYPE
};

#define SCROLL_DIRECTION ScrollDirection
#define EVENT_TYPE EventType

#include "Shared/Event.h"
