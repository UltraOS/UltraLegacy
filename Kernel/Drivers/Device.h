#pragma once
#include "Common/String.h"

namespace kernel {

class Device {
    MAKE_NONCOPYABLE(Device);

public:
    enum class Category : u32 {
        TIMER,
        INTERRUPT_CONTROLLER,
        VIDEO,
        KEYBOARD,
        MOUSE,
        STORAGE,
        NETWORK,
        AUDIO,
        CONTROLLER,

        LAST
    };

    Device(Category);

    Category device_category() const { return m_category; }
    virtual StringView device_type() const = 0;
    virtual bool matches_type(StringView type) { return device_type() == type; }
    virtual StringView device_model() const = 0;

    static StringView category_to_string(Category category)
    {
        switch (category) {
        case Category::TIMER:
            return "Timer"_sv;
        case Category::INTERRUPT_CONTROLLER:
            return "Interrupt Controller"_sv;
        case Category::VIDEO:
            return "Video"_sv;
        case Category::KEYBOARD:
            return "Keyboard"_sv;
        case Category::MOUSE:
            return "Mouse"_sv;
        case Category::STORAGE:
            return "Storage"_sv;
        case Category::NETWORK:
            return "Network"_sv;
        case Category::AUDIO:
            return "Audio"_sv;
        case Category::CONTROLLER:
            return "Controller"_sv;
        default:
            return "<Invalid>"_sv;
        }
    }

    StringView category_to_string() const { return category_to_string(device_category()); }

    void make_child_of(Device*);
    bool is_primary();
    void make_primary();

    virtual ~Device();

private:
    Category m_category;
};
}
