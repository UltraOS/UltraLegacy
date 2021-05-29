#pragma once

#include "Common/String.h"
#include "Shared/VirtualKey.h"

enum class VK : uint8_t {
#define VIRTUAL_KEY(key, representation) key,
    ENUMERATE_VIRTUAL_KEYS
#undef VIRTUAL_KEY
};

#define VK VK

enum class VKState : uint8_t {
#define VK_STATE(state) state,
    ENUMERATE_VK_STATES
#undef VK_STATE
};

#define VK_STATE VKState

namespace kernel {

inline StringView to_string(VK k)
{
    switch (k) {
#define VIRTUAL_KEY(key, representation) \
    case VK::key:                        \
        return representation##_sv;
        ENUMERATE_VIRTUAL_KEYS
#undef VIRTUAL_KEY
    default:
        return "Unknown"_sv;
    }
}

inline char to_char(VK key, VKState shift_state, VKState caps_state, bool& did_convert)
{
    did_convert = true;
    bool shift_pressed = shift_state == VKState::PRESSED;
    bool caps_pressed = caps_state == VKState::PRESSED;

    bool should_capitalize = (shift_pressed && !caps_pressed) || (caps_pressed && !shift_pressed);

    switch (key) {
    case VK::ONE:
        if (shift_pressed)
            return '!';
        else
            return '1';
    case VK::TWO:
        if (shift_pressed)
            return '@';
        else
            return '2';

    case VK::THREE:
        if (shift_pressed)
            return '#';
        else
            return '3';

    case VK::FOUR:
        if (shift_pressed)
            return '$';
        else
            return '4';

    case VK::FIVE:
        if (shift_pressed)
            return '%';
        else
            return '5';

    case VK::SIX:
        if (shift_pressed)
            return '^';
        else
            return '6';

    case VK::SEVEN:
        if (shift_pressed)
            return '&';
        else
            return '7';

    case VK::EIGHT:
        if (shift_pressed)
            return '&';
        else
            return '8';

    case VK::NINE:
        if (shift_pressed)
            return '(';
        else
            return '9';

    case VK::ZERO:
        if (shift_pressed)
            return ')';
        else
            return '0';

    case VK::HYPHEN:
        if (shift_pressed)
            return '_';
        else
            return '-';

    case VK::EQUAL:
        if (shift_pressed)
            return '+';
        else
            return '=';

    case VK::Q:
        if (should_capitalize)
            return 'Q';
        else
            return 'q';

    case VK::W:
        if (should_capitalize)
            return 'W';
        else
            return 'w';

    case VK::E:
        if (should_capitalize)
            return 'E';
        else
            return 'e';

    case VK::R:
        if (should_capitalize)
            return 'R';
        else
            return 'r';

    case VK::T:
        if (should_capitalize)
            return 'T';
        else
            return 't';

    case VK::Y:
        if (should_capitalize)
            return 'Y';
        else
            return 'y';

    case VK::U:
        if (should_capitalize)
            return 'U';
        else
            return 'u';

    case VK::I:
        if (should_capitalize)
            return 'I';
        else
            return 'i';

    case VK::O:
        if (should_capitalize)
            return 'O';
        else
            return 'o';

    case VK::P:
        if (should_capitalize)
            return 'P';
        else
            return 'p';

    case VK::OPEN_BRACKET:
        if (shift_pressed)
            return '{';
        else
            return '[';

    case VK::CLOSE_BRACKET:
        if (shift_pressed)
            return '}';
        else
            return ']';

    case VK::BACKSLASH:
        if (shift_pressed)
            return '|';
        else
            return '\\';

    case VK::NP_SEVEN:
        return '7';
    case VK::NP_EIGHT:
        return '8';
    case VK::NP_NINE:
        return '9';
    case VK::NP_PLUS:
        return '+';

    case VK::A:
        if (should_capitalize)
            return 'A';
        else
            return 'a';
    case VK::S:
        if (should_capitalize)
            return 'S';
        else
            return 's';

    case VK::D:
        if (should_capitalize)
            return 'D';
        else
            return 'd';

    case VK::F:
        if (should_capitalize)
            return 'F';
        else
            return 'f';

    case VK::G:
        if (should_capitalize)
            return 'G';
        else
            return 'g';
    case VK::H:
        if (should_capitalize)
            return 'H';
        else
            return 'h';

    case VK::J:
        if (should_capitalize)
            return 'J';
        else
            return 'j';

    case VK::K:
        if (should_capitalize)
            return 'K';
        else
            return 'k';

    case VK::L:
        if (should_capitalize)
            return 'L';
        else
            return 'l';

    case VK::SEMICOLON:
        if (shift_pressed)
            return ':';
        else
            return ';';

    case VK::APOSTROPHE:
        if (shift_pressed)
            return '\"';
        else
            return '\'';

    case VK::NP_FOUR:
        return '4';
    case VK::NP_FIVE:
        return '5';
    case VK::NP_SIX:
        return '6';

    case VK::Z:
        if (should_capitalize)
            return 'Z';
        else
            return 'z';

    case VK::X:
        if (should_capitalize)
            return 'X';
        else
            return 'x';

    case VK::C:
        if (should_capitalize)
            return 'C';
        else
            return 'c';

    case VK::V:
        if (should_capitalize)
            return 'V';
        else
            return 'v';

    case VK::B:
        if (should_capitalize)
            return 'B';
        else
            return 'b';
    case VK::N:
        if (should_capitalize)
            return 'N';
        else
            return 'n';

    case VK::M:
        if (should_capitalize)
            return 'M';
        else
            return 'm';

    case VK::COMMA:
        if (shift_pressed)
            return '<';
        else
            return ',';

    case VK::DOT:
        if (shift_pressed)
            return '>';
        else
            return '.';

    case VK::FORWARD_SLASH:
        if (shift_pressed)
            return '?';
        else
            return '/';

    case VK::SPACE:
        return ' ';

    case VK::TAB:
        return '\t';

    default:
        did_convert = false;
        return static_cast<char>(-1);
    }
}

}
