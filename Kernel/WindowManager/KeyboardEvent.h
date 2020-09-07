#pragma once

#include "Event.h"
#include "VirtualKey.h"

class KeyboardEvent : public Event {
public:
    Category category() const override { return Category::KeyboardEvent; }

    virtual ~KeyboardEvent() = default;
};

class KeyEvent : public KeyboardEvent {
public:
    KeyEvent(Key key) : m_key(key) { }

    virtual bool is_pressed() const = 0;

    bool is_released() const { return !is_pressed(); }

    Key key() const { return m_key; }

private:
    Key m_key;
};

class KeyPressEvent : public KeyEvent {
public:
    KeyPressEvent(Key key) : KeyEvent(key) { }

    Type type() const override { return Type::KeyPress; }

    bool is_pressed() const override { return true; }
};

class KeyReleaseEvent : public KeyEvent {
public:
    KeyReleaseEvent(Key key) : KeyEvent(key) { }

    Type type() const override { return Type::KeyRelease; }

    bool is_pressed() const override { return false; }
};
