#pragma once

#include <stdint.h>

#ifndef SCROLL_DIRECTION
#define SCROLL_DIRECTION uint8_t
#endif

#ifndef VK
#define VK uint8_t
#endif

#ifndef VK_STATE
#define VK_STATE uint8_t
#endif

#ifndef EVENT_TYPE
#define EVENT_TYPE uint8_t
#endif

typedef struct {
    union {
        struct {
            size_t x;
            size_t y;
        } mouse_move;

        struct {
            SCROLL_DIRECTION direction;
            int8_t delta;
        } mouse_scroll;

        struct {
            VK vkey;
            VK_STATE state;
        } vk_state;

        struct {
            size_t character;
        } char_typed;
    };

    EVENT_TYPE type;
} Event;
