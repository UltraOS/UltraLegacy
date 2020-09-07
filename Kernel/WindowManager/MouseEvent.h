#pragma once

#include "Event.h"
#include "VirtualKey.h"

class MouseEvent : public Event {
public:
    Category category() const override { return Category::MouseEvent; }

    virtual ~MouseEvent() = default;
};

class MouseMoveEvent : public MouseEvent {
public:
    MouseMoveEvent(size_t x, size_t y) : m_x(x), m_y(y) { }

    Type type() const override { return Type::MouseMove; }

    size_t x() const { return m_x; }
    size_t y() const { return m_y; }

private:
    size_t m_x;
    size_t m_y;
};

class ButtonEvent : public MouseEvent {
public:
    ButtonEvent(VK key) : m_button(key) { }

    virtual bool is_pressed() const = 0;
    bool         is_released() const { return !is_pressed(); }

    VK button() const { return m_button; }

private:
    VK m_button;
};

class ButtonPressEvent : public ButtonEvent {
    ButtonPressEvent(VK button) : ButtonEvent(button) { }

    Type type() const override { return Type::ButtonPress; }

    bool is_pressed() const override { return true; }
};

class ButtonReleaseEvent : public ButtonEvent {
    ButtonReleaseEvent(VK button) : ButtonEvent(button) { }

    Type type() const override { return Type::ButtonRelease; }

    bool is_pressed() const override { return false; }
};

class MouseScrollEvent : public MouseEvent {
public:
    Type type() const override { return Type::MouseScroll; }

    enum class Direction : unsigned char { Vertical, Horizontal };

    MouseScrollEvent(Direction direction, signed char delta) : m_direction(direction), m_delta(delta) { }

    Direction direction() const { return m_direction; }

    signed char delta() { return m_delta; }

private:
    Direction   m_direction;
    signed char m_delta;
};
