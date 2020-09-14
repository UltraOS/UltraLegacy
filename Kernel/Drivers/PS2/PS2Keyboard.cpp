#include "PS2Keyboard.h"
#include "Common/Logger.h"
#include "WindowManager/EventManager.h"
#include "WindowManager/VirtualKey.h"

namespace kernel {

static const VK single_byte_keys[] = {
    VK::UNKNOWN,
    VK::ESC,
    VK::ONE,
    VK::TWO,
    VK::THREE,
    VK::FOUR,
    VK::FIVE,
    VK::SIX,
    VK::SEVEN,
    VK::EIGHT,
    VK::NINE,
    VK::ZERO,
    VK::HYPHEN,
    VK::EQUAL,
    VK::BACKSPACE,
    VK::TAB,
    VK::Q,
    VK::W,
    VK::E,
    VK::R,
    VK::T,
    VK::Y,
    VK::U,
    VK::I,
    VK::O,
    VK::P,
    VK::OPEN_BRACKET,
    VK::CLOSE_BRACKET,
    VK::ENTER,
    VK::LEFT_CTRL,
    VK::A,
    VK::S,
    VK::D,
    VK::F,
    VK::G,
    VK::H,
    VK::J,
    VK::K,
    VK::L,
    VK::SEMICOLON,
    VK::APOSTROPHE,
    VK::BACKTICK,
    VK::LEFT_SHIFT,
    VK::BACKSLASH,
    VK::Z,
    VK::X,
    VK::C,
    VK::V,
    VK::B,
    VK::N,
    VK::M,
    VK::COMMA,
    VK::DOT,
    VK::FORWARD_SLASH,
    VK::RIGHT_SHIFT,
    VK::NP_ASTERISK,
    VK::LEFT_ALT,
    VK::SPACE,
    VK::CAPS_LOCK,
    VK::F1,
    VK::F2,
    VK::F3,
    VK::F4,
    VK::F5,
    VK::F6,
    VK::F7,
    VK::F8,
    VK::F9,
    VK::F10,
    VK::NUM_LK,
    VK::SCR_LK,
    VK::NP_SEVEN,
    VK::NP_EIGHT,
    VK::NP_NINE,
    VK::NP_HYPHEN,
    VK::NP_FOUR,
    VK::NP_FIVE,
    VK::NP_SIX,
    VK::NP_PLUS,
    VK::NP_ONE,
    VK::NP_TWO,
    VK::NP_THREE,
    VK::NP_ZERO,
    VK::NP_DOT, // 0x53
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::F11,
    VK::F12,
};

static const VK multi_byte_keys[] = {
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0xF
    VK::MM_PREVIOUS_TRACK,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x18
    VK::MM_NEXT_TRACK,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x1B
    VK::NP_ENTER,
    VK::RIGHT_CTRL,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x1F
    VK::MM_MUTE,
    VK::MM_CALCULATOR,
    VK::MM_PLAY,
    VK::UNKNOWN, // 0x23
    VK::MM_STOP,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x2D
    VK::MM_VOLUME_DOWN,
    VK::UNKNOWN, // 0x2F
    VK::MM_VOLUME_UP,
    VK::UNKNOWN, // 0x31
    VK::MM_BROWSER_HOME,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x34
    VK::NP_FORWARD_SLASH,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x37
    VK::RIGHT_ALT,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x46
    VK::HOME,
    VK::ARROW_UP,
    VK::PAGE_UP,
    VK::UNKNOWN, // 0x4A
    VK::ARROW_LEFT,
    VK::UNKNOWN, // 0x4C
    VK::ARROW_RIGHT,
    VK::UNKNOWN, // 0x4E
    VK::END,
    VK::ARROW_DOWN,
    VK::PAGE_DOWN,
    VK::INSERT,
    VK::DEL,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x5A
    VK::LEFT_LOGO,
    VK::RIGHT_LOGO,
    VK::CONTEXT_MENU,
    VK::POWER,
    VK::SLEEP,
    VK::UNKNOWN,
    VK::UNKNOWN,
    VK::UNKNOWN, // 0x62
    VK::WAKE,
    VK::UNKNOWN, // 0x64
    VK::MM_BROWSER_SEARCH,
    VK::MM_BROWSER_FAVORITES,
    VK::MM_BROWSER_REFRESH,
    VK::MM_BROWSER_STOP,
    VK::MM_BROWSER_FORWARD,
    VK::MM_BROWSER_BACK,
    VK::MM_MY_COMPUTER,
    VK::MM_EMAIL,
    VK::MM_MEDIA_SELECT,
};

PS2Keyboard::PS2Keyboard(PS2Controller::Channel channel) : PS2Device(channel)
{
    enable_irq();
}

void PS2Keyboard::handle_action()
{
    while (data_available()) {
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
                EventManager::the().post_action({ VK::PRT_SC, VKState::PRESSED });
            else if (scancode == prt_sc_released_code) {
                EventManager::the().post_action({ VK::PRT_SC, VKState::RELEASED });
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
                EventManager::the().post_action({ VK::PAUSE_BREAK, VKState::PRESSED });
                EventManager::the().post_action({ VK::PAUSE_BREAK, VKState::RELEASED });
            }

            m_state = State::NORMAL;
            continue;
        }

        bool released = scancode & key_released_bit;
        auto raw_key  = scancode & raw_key_mask;

        if (m_state == State::NORMAL) {
            EventManager::the().post_action(
                { single_byte_keys[raw_key], released ? VKState::RELEASED : VKState::PRESSED });
        } else if (m_state == State::E0) {
            EventManager::the().post_action(
                { multi_byte_keys[raw_key], released ? VKState::RELEASED : VKState::PRESSED });
        } else {
            warning() << "PS2Keyboard: unexpected state " << static_cast<u8>(m_state);
        }

        m_state = State::NORMAL;
    }
}
}
