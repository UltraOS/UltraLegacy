#include "PS2Keyboard.h"
#include "Common/Logger.h"
#include "Drivers/Key.h"

namespace kernel {

static const Key single_byte_keys[] = {
    Key::UNKNOWN,
    Key::ESC,
    Key::ONE,
    Key::TWO,
    Key::THREE,
    Key::FOUR,
    Key::FIVE,
    Key::SIX,
    Key::SEVEN,
    Key::EIGHT,
    Key::NINE,
    Key::ZERO,
    Key::HYPHEN,
    Key::EQUAL,
    Key::BACKSPACE,
    Key::TAB,
    Key::Q,
    Key::W,
    Key::E,
    Key::R,
    Key::T,
    Key::Y,
    Key::U,
    Key::I,
    Key::O,
    Key::P,
    Key::OPEN_BRACKET,
    Key::CLOSE_BRACKET,
    Key::ENTER,
    Key::LEFT_CTRL,
    Key::A,
    Key::S,
    Key::D,
    Key::F,
    Key::G,
    Key::H,
    Key::J,
    Key::K,
    Key::L,
    Key::SEMICOLON,
    Key::APOSTROPHE,
    Key::BACKTICK,
    Key::LEFT_SHIFT,
    Key::BACKSLASH,
    Key::Z,
    Key::X,
    Key::C,
    Key::V,
    Key::B,
    Key::N,
    Key::M,
    Key::COMMA,
    Key::DOT,
    Key::FORWARD_SLASH,
    Key::RIGHT_SHIFT,
    Key::NP_ASTERISK,
    Key::LEFT_ALT,
    Key::SPACE,
    Key::CAPS_LOCK,
    Key::F1,
    Key::F2,
    Key::F3,
    Key::F4,
    Key::F5,
    Key::F6,
    Key::F7,
    Key::F8,
    Key::F9,
    Key::F10,
    Key::NUM_LK,
    Key::SCR_LK,
    Key::NP_SEVEN,
    Key::NP_EIGHT,
    Key::NP_NINE,
    Key::NP_HYPHEN,
    Key::NP_FOUR,
    Key::NP_FIVE,
    Key::NP_SIX,
    Key::NP_PLUS,
    Key::NP_ONE,
    Key::NP_TWO,
    Key::NP_THREE,
    Key::NP_ZERO,
    Key::NP_DOT, // 0x53
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::F11,
    Key::F12,
};

static const Key multi_byte_keys[] = {
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0xF
    Key::MM_PREVIOUS_TRACK,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x18
    Key::MM_NEXT_TRACK,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x1B
    Key::NP_ENTER,
    Key::RIGHT_CTRL,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x1F
    Key::MM_MUTE,
    Key::MM_CALCULATOR,
    Key::MM_PLAY,
    Key::UNKNOWN, // 0x23
    Key::MM_STOP,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x2D
    Key::MM_VOLUME_DOWN,
    Key::UNKNOWN, // 0x2F
    Key::MM_VOLUME_UP,
    Key::UNKNOWN, // 0x31
    Key::MM_BROWSER_HOME,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x34
    Key::NP_FORWARD_SLASH,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x37
    Key::RIGHT_ALT,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x46
    Key::HOME,
    Key::ARROW_UP,
    Key::PAGE_UP,
    Key::UNKNOWN, // 0x4A
    Key::ARROW_LEFT,
    Key::UNKNOWN, // 0x4C
    Key::ARROW_RIGHT,
    Key::UNKNOWN, // 0x4E
    Key::END,
    Key::ARROW_DOWN,
    Key::PAGE_DOWN,
    Key::INSERT,
    Key::DEL,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x5A
    Key::LEFT_LOGO,
    Key::RIGHT_LOGO,
    Key::CONTEXT_MENU,
    Key::POWER,
    Key::SLEEP,
    Key::UNKNOWN,
    Key::UNKNOWN,
    Key::UNKNOWN, // 0x62
    Key::WAKE,
    Key::UNKNOWN, // 0x64
    Key::MM_BROWSER_SEARCH,
    Key::MM_BROWSER_FAVORITES,
    Key::MM_BROWSER_REFRESH,
    Key::MM_BROWSER_STOP,
    Key::MM_BROWSER_FORWARD,
    Key::MM_BROWSER_BACK,
    Key::MM_MY_COMPUTER,
    Key::MM_EMAIL,
    Key::MM_MEDIA_SELECT,
};

PS2Keyboard::PS2Keyboard(PS2Controller::Channel channel) : PS2Device(channel)
{
    enable_irq();
}

void PS2Keyboard::handle_action()
{
    while (PS2Controller::the().status().output_full) {
        static constexpr u8 key_released_bit = SET_BIT(7);
        static constexpr u8 raw_key_mask     = SET_BIT(7) - 1;

        auto scancode = read_data();

        switch (m_state) {
        case State::E0:
            if (scancode == 0x2A || scancode == 0xB7) {
                m_state = State::PRINT_SCREEN_1;
                continue;
            }
            break;
        case State::PRINT_SCREEN_1:
            if (scancode == 0xE0) {
                m_state = State::E0_REPEAT;
                continue;
            }
            m_state = State::NORMAL;
            break;
        case State::E0_REPEAT:
            static constexpr u8 prt_sc_pressed_code  = 0x37;
            static constexpr u8 prt_sc_released_code = 0xAA;

            if (scancode == prt_sc_pressed_code)
                log() << "PS2Keyboard: print screen pressed";
            else if (scancode == prt_sc_released_code) {
                log() << "PS2Keyboard: print screen released";
            } else
                warning() << "PS2Keyboard: expected a prt sc scancode 0x37/0xAA, got " << format::as_hex << scancode;

            m_state = State::NORMAL;
            continue;
        case State::NORMAL:
            if (scancode == 0xE0)
                m_state = State::E0;
            else if (scancode == 0xE1)
                m_state = State::E1;
            else
                break;

            continue;
        case State::E1:
            if (scancode == 0x1D)
                m_state = State::PAUSE_1;
            continue;
        case State::PAUSE_1:
            if (scancode == 0x45)
                m_state = State::PAUSE_2;
            continue;
        case State::PAUSE_2:
            if (scancode == 0xE1)
                m_state = State::E1_REPEAT;
            continue;
        case State::E1_REPEAT:
            if (scancode == 0x9D)
                m_state = State::PAUSE_3;
            continue;
        case State::PAUSE_3:
            if (scancode == 0xC5) {
                log() << "PS2Keyboard: pause pressed";
            }
            m_state = State::NORMAL;
            continue;
        }

        bool released = scancode & key_released_bit;
        auto raw_key  = scancode & raw_key_mask;

        if (m_state == State::NORMAL) {
            log() << "PS2Keyboard: key " << to_string(single_byte_keys[raw_key])
                  << (released ? " released" : " pressed");
        } else {
            log() << "PS2Keyboard: key " << to_string(multi_byte_keys[raw_key])
                  << (released ? " released" : " pressed");
        }

        m_state = State::NORMAL;
    }
}
}
