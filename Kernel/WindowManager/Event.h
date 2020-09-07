#pragma once

class Event {
public:
    enum class Type : unsigned char {
        MouseMove,
        ButtonPress,
        ButtonRelease,
        MouseScroll,
        KeyPress,
        KeyRelease,
        WindowResize,
        WindowMove,
    };

    enum class Category : unsigned char {
        MouseEvent,
        KeyboardEvent,
        WindowEvent,
    };

    virtual Type     type() const     = 0;
    virtual Category category() const = 0;
};
