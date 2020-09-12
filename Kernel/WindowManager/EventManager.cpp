#include "EventManager.h"
#include "Common/Logger.h"
#include "Screen.h"
#include "Window.h"

namespace kernel {

EventManager EventManager::s_instance;

void EventManager::update_state_of_key(VK key, VKState state)
{
    ASSERT(static_cast<u8>(key) < static_cast<u8>(VK::LAST));

    m_key_state[static_cast<u8>(key)] = state;
}

void EventManager::generate_button_state_event(VK key, VKState state)
{
    update_state_of_key(key, state);

    if (!Window::is_any_focused())
        return;

    Event e {};
    e.type     = Event::Type::BUTTON_STATE;
    e.vk_state = { key, state };

    Window::focused().enqueue_event(e);
}

void EventManager::post_action(const Keyboard::Packet& packet)
{
    log() << "KeyPress: " << to_string(packet.key) << " is_press: " << (packet.state == VKState::PRESSED);

    update_state_of_key(packet.key, packet.state);

    if (!Window::is_any_focused())
        return;

    Event e {};
    e.type     = Event::Type::KEY_STATE;
    e.vk_state = { packet.key, packet.state };

    Window::focused().enqueue_event(e);
}

void EventManager::post_action(const Mouse::Packet& packet)
{
    // detect changes of state
    if (packet.x_delta || packet.y_delta) {
        log() << "MouseMove: x " << packet.x_delta << " y " << packet.y_delta;

        Screen::the().recalculate_cursor_position(packet.x_delta, packet.y_delta);
        log() << "MouseLocation: " << Screen::the().cursor().location().x() << "x" << Screen::the().cursor().location().y();

        Event e {};

        e.type = Event::Type::MOUSE_MOVE;

        // calculate location relative to window
        e.mouse_move = { 0, 0 };

        if (!Window::is_any_focused())
            return;

        Window::focused().enqueue_event(e);
    }

    if (packet.wheel_delta) {
        log() << "MouseScroll: delta " << packet.wheel_delta << " direction "
              << (packet.scroll_direction == ScrollDirection::VERTICAL ? "vertical" : "horizontal");

        if (!Window::is_any_focused())
            return;

        Event e {};

        e.type         = Event::Type::MOUSE_MOVE;
        e.mouse_scroll = { packet.scroll_direction, packet.wheel_delta };

        Window::focused().enqueue_event(e);
    }

    if (packet.left_button_state != m_last_mouse_state.left_button_state) {
        generate_button_state_event(VK::MOUSE_LEFT_BUTTON, packet.left_button_state);
        log() << "ButtonPress: left is_press: " << (packet.left_button_state == VKState::PRESSED);
    }
    if (packet.middle_button_state != m_last_mouse_state.middle_button_state) {
        generate_button_state_event(VK::MOUSE_MIDDLE_BUTTON, packet.middle_button_state);
        log() << "ButtonPress: middle is_press: " << (packet.middle_button_state == VKState::PRESSED);
    }
    if (packet.right_button_state != m_last_mouse_state.right_button_state) {
        generate_button_state_event(VK::MOUSE_RIGHT_BUTTON, packet.right_button_state);
        log() << "ButtonPress: right is_press: " << (packet.right_button_state == VKState::PRESSED);
    }
    if (packet.button_4_state != m_last_mouse_state.button_4_state) {
        generate_button_state_event(VK::MOUSE_BUTTON_4, packet.button_4_state);
        log() << "ButtonPress: 4 is_press: " << (packet.button_4_state == VKState::PRESSED);
    }
    if (packet.button_5_state != m_last_mouse_state.button_5_state) {
        generate_button_state_event(VK::MOUSE_BUTTON_5, packet.button_5_state);
        log() << "ButtonPress: 5 is_press: " << (packet.button_5_state == VKState::PRESSED);
    }

    m_last_mouse_state = packet;
}
}
