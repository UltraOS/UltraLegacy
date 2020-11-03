#pragma once

#include "VirtualKey.h"

enum class ScrollDirection : uint8_t {
    HORIZONTAL = 0,
    VERTICAL = 1
};

struct Event {
    union {
        struct {
            size_t x;
            size_t y;
        } mouse_move;

        struct {
            ScrollDirection direction;
            int8_t delta;
        } mouse_scroll;

        struct {
            VK vkey;
            VKState state;
        } vk_state;

        struct {
            size_t character;
        } char_typed;
    };

    using MouseMove = decltype(mouse_move);
    using MouseScroll = decltype(mouse_scroll);
    using VKStateChange = decltype(vk_state);

    enum class Type : uint8_t {
        MOUSE_MOVE,
        BUTTON_STATE,
        MOUSE_SCROLL,
        KEY_STATE,
        WINDOW_RESIZE,
        WINDOW_MOVE,
        CHAR_TYPED,
    } type;
};
