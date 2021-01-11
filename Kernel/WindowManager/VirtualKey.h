#pragma once

#include "Common/String.h"

#define ENUMERATE_VIRTUAL_KEYS                              \
    VIRTUAL_KEY(UNKNOWN, "Unknown")                         \
    VIRTUAL_KEY(ESC, "Escape")                              \
    VIRTUAL_KEY(F1, "F1")                                   \
    VIRTUAL_KEY(F2, "F2")                                   \
    VIRTUAL_KEY(F3, "F3")                                   \
    VIRTUAL_KEY(F4, "F4")                                   \
    VIRTUAL_KEY(F5, "F5")                                   \
    VIRTUAL_KEY(F6, "F6")                                   \
    VIRTUAL_KEY(F7, "F7")                                   \
    VIRTUAL_KEY(F8, "F8")                                   \
    VIRTUAL_KEY(F9, "F9")                                   \
    VIRTUAL_KEY(F10, "F10")                                 \
    VIRTUAL_KEY(F11, "F11")                                 \
    VIRTUAL_KEY(F12, "F12")                                 \
    VIRTUAL_KEY(PRT_SC, "Print Screen")                     \
    VIRTUAL_KEY(SCR_LK, "Scroll Lock:")                     \
    VIRTUAL_KEY(PAUSE_BREAK, "Pause Break")                 \
    VIRTUAL_KEY(BACKTICK, "`")                              \
    VIRTUAL_KEY(ONE, "1")                                   \
    VIRTUAL_KEY(TWO, "2")                                   \
    VIRTUAL_KEY(THREE, "3")                                 \
    VIRTUAL_KEY(FOUR, "4")                                  \
    VIRTUAL_KEY(FIVE, "5")                                  \
    VIRTUAL_KEY(SIX, "6")                                   \
    VIRTUAL_KEY(SEVEN, "7")                                 \
    VIRTUAL_KEY(EIGHT, "8")                                 \
    VIRTUAL_KEY(NINE, "9")                                  \
    VIRTUAL_KEY(ZERO, "0")                                  \
    VIRTUAL_KEY(HYPHEN, "-")                                \
    VIRTUAL_KEY(EQUAL, "=")                                 \
    VIRTUAL_KEY(BACKSPACE, "Backspace")                     \
    VIRTUAL_KEY(INSERT, "Insert")                           \
    VIRTUAL_KEY(HOME, "Home")                               \
    VIRTUAL_KEY(PAGE_UP, "Page Up")                         \
    VIRTUAL_KEY(NUM_LK, "Num Lock")                         \
    VIRTUAL_KEY(NP_FORWARD_SLASH, "/")                      \
    VIRTUAL_KEY(NP_ASTERISK, "*")                           \
    VIRTUAL_KEY(NP_HYPHEN, "-")                             \
    VIRTUAL_KEY(TAB, "Tab")                                 \
    VIRTUAL_KEY(Q, "Q")                                     \
    VIRTUAL_KEY(W, "W")                                     \
    VIRTUAL_KEY(E, "E")                                     \
    VIRTUAL_KEY(R, "R")                                     \
    VIRTUAL_KEY(T, "T")                                     \
    VIRTUAL_KEY(Y, "Y")                                     \
    VIRTUAL_KEY(U, "U")                                     \
    VIRTUAL_KEY(I, "I")                                     \
    VIRTUAL_KEY(O, "O")                                     \
    VIRTUAL_KEY(P, "P")                                     \
    VIRTUAL_KEY(OPEN_BRACKET, "[")                          \
    VIRTUAL_KEY(CLOSE_BRACKET, "]")                         \
    VIRTUAL_KEY(BACKSLASH, "\\")                            \
    VIRTUAL_KEY(PIPE, "|")                                  \
    VIRTUAL_KEY(DEL, "Del")                                 \
    VIRTUAL_KEY(END, "End")                                 \
    VIRTUAL_KEY(PAGE_DOWN, "Page Down")                     \
    VIRTUAL_KEY(NP_SEVEN, "7")                              \
    VIRTUAL_KEY(NP_EIGHT, "8")                              \
    VIRTUAL_KEY(NP_NINE, "9")                               \
    VIRTUAL_KEY(NP_PLUS, "+")                               \
    VIRTUAL_KEY(CAPS_LOCK, "Caps Lock")                     \
    VIRTUAL_KEY(A, "A")                                     \
    VIRTUAL_KEY(S, "S")                                     \
    VIRTUAL_KEY(D, "D")                                     \
    VIRTUAL_KEY(F, "F")                                     \
    VIRTUAL_KEY(G, "G")                                     \
    VIRTUAL_KEY(H, "H")                                     \
    VIRTUAL_KEY(J, "J")                                     \
    VIRTUAL_KEY(K, "K")                                     \
    VIRTUAL_KEY(L, "L")                                     \
    VIRTUAL_KEY(SEMICOLON, ";")                             \
    VIRTUAL_KEY(APOSTROPHE, "'")                            \
    VIRTUAL_KEY(ENTER, "Enter")                             \
    VIRTUAL_KEY(NP_FOUR, "4")                               \
    VIRTUAL_KEY(NP_FIVE, "5")                               \
    VIRTUAL_KEY(NP_SIX, "6")                                \
    VIRTUAL_KEY(LEFT_SHIFT, "Left Shift")                   \
    VIRTUAL_KEY(Z, "Z")                                     \
    VIRTUAL_KEY(X, "X")                                     \
    VIRTUAL_KEY(C, "C")                                     \
    VIRTUAL_KEY(V, "V")                                     \
    VIRTUAL_KEY(B, "B")                                     \
    VIRTUAL_KEY(N, "N")                                     \
    VIRTUAL_KEY(M, "M")                                     \
    VIRTUAL_KEY(COMMA, ",")                                 \
    VIRTUAL_KEY(DOT, ".")                                   \
    VIRTUAL_KEY(FORWARD_SLASH, "/")                         \
    VIRTUAL_KEY(RIGHT_SHIFT, "Right Shift")                 \
    VIRTUAL_KEY(ARROW_UP, "Arrow Up")                       \
    VIRTUAL_KEY(NP_ONE, "1")                                \
    VIRTUAL_KEY(NP_TWO, "2")                                \
    VIRTUAL_KEY(NP_THREE, "3")                              \
    VIRTUAL_KEY(NP_ENTER, "Enter")                          \
    VIRTUAL_KEY(LEFT_CTRL, "Left Ctrl")                     \
    VIRTUAL_KEY(LEFT_LOGO, "Left Logo")                     \
    VIRTUAL_KEY(LEFT_ALT, "Left Alt")                       \
    VIRTUAL_KEY(SPACE, "Space")                             \
    VIRTUAL_KEY(RIGHT_ALT, "Right Alt")                     \
    VIRTUAL_KEY(RIGHT_LOGO, "Right Logo")                   \
    VIRTUAL_KEY(CONTEXT_MENU, "Context Menu")               \
    VIRTUAL_KEY(RIGHT_CTRL, "Right Ctrl")                   \
    VIRTUAL_KEY(ARROW_LEFT, "Arrow Left")                   \
    VIRTUAL_KEY(ARROW_DOWN, "Arrow Down")                   \
    VIRTUAL_KEY(ARROW_RIGHT, "Arrow Right")                 \
    VIRTUAL_KEY(NP_ZERO, "0")                               \
    VIRTUAL_KEY(NP_DOT, ".")                                \
    VIRTUAL_KEY(MM_PREVIOUS_TRACK, "Previous Track")        \
    VIRTUAL_KEY(MM_NEXT_TRACK, "Next Track")                \
    VIRTUAL_KEY(MM_MUTE, "Mute")                            \
    VIRTUAL_KEY(MM_CALCULATOR, "Calculator")                \
    VIRTUAL_KEY(MM_PLAY, "Play")                            \
    VIRTUAL_KEY(MM_STOP, "Stop")                            \
    VIRTUAL_KEY(MM_VOLUME_DOWN, "Volume Down")              \
    VIRTUAL_KEY(MM_VOLUME_UP, "Volume Up")                  \
    VIRTUAL_KEY(MM_BROWSER_HOME, "Browser Home")            \
    VIRTUAL_KEY(POWER, "Power")                             \
    VIRTUAL_KEY(SLEEP, "Sleep")                             \
    VIRTUAL_KEY(WAKE, "Wake")                               \
    VIRTUAL_KEY(MM_BROWSER_SEARCH, "Browser Search")        \
    VIRTUAL_KEY(MM_BROWSER_FAVORITES, "Browser Favorites")  \
    VIRTUAL_KEY(MM_BROWSER_REFRESH, "Browser Refresh")      \
    VIRTUAL_KEY(MM_BROWSER_STOP, "Browser Stop")            \
    VIRTUAL_KEY(MM_BROWSER_FORWARD, "Browser Forward")      \
    VIRTUAL_KEY(MM_BROWSER_BACK, "Browser Back")            \
    VIRTUAL_KEY(MM_MY_COMPUTER, "My Computer")              \
    VIRTUAL_KEY(MM_EMAIL, "Email")                          \
    VIRTUAL_KEY(MM_MEDIA_SELECT, "Media Select")            \
    VIRTUAL_KEY(MOUSE_LEFT_BUTTON, "Left Mouse Button")     \
    VIRTUAL_KEY(MOUSE_RIGHT_BUTTON, "Right Mouse Button")   \
    VIRTUAL_KEY(MOUSE_MIDDLE_BUTTON, "Middle Mouse Button") \
    VIRTUAL_KEY(MOUSE_BUTTON_4, "Mouse Button 4")           \
    VIRTUAL_KEY(MOUSE_BUTTON_5, "Mouse Button 5")           \
    VIRTUAL_KEY(LAST, "A VK with the highest possible value, no other VK can be bigger")

enum class VK : uint8_t {
#define VIRTUAL_KEY(key, representation) key,
    ENUMERATE_VIRTUAL_KEYS
#undef VIRTUAL_KEY
};

enum class VKState : uint8_t {
    RELEASED = 0,
    PRESSED = 1
};

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
