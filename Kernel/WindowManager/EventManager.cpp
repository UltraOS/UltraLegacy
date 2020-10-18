#include "EventManager.h"
#include "Common/Logger.h"
#include "Screen.h"
#include "Window.h"

// #define EVENT_MANAGER_DEBUG

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

    LockGuard lock_guard(Window::focus_lock());

    if (state != VKState::RELEASED)
        Screen::the().check_if_focused_window_should_change();

    if (!Window::is_any_focused())
        return;

    Event e {};
    e.type     = Event::Type::BUTTON_STATE;
    e.vk_state = { key, state };

    Window::focused().handle_event(e);
}

void EventManager::post_action(const Keyboard::Packet& packet)
{
#ifdef EVENT_MANAGER_DEBUG
    log() << "EventManager: key press " << to_string(packet.key) << " is_press: " << (packet.state == VKState::PRESSED);
#endif

    update_state_of_key(packet.key, packet.state);

    LockGuard lock_guard(Window::focus_lock());

    if (!Window::is_any_focused())
        return;

    Event e {};
    e.type     = Event::Type::KEY_STATE;
    e.vk_state = { packet.key, packet.state };

    Window::focused().handle_event(e);
}

void EventManager::post_action(const Mouse::Packet& packet)
{
    // detect changes of state
    if (packet.x_delta || packet.y_delta) {
#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: mouse move x " << packet.x_delta << " y " << packet.y_delta;
#endif

        Screen::the().recalculate_cursor_position(packet.x_delta, packet.y_delta);

        Event e {};

        e.type = Event::Type::MOUSE_MOVE;

        auto loc = Screen::the().cursor().location();

        // TODO: calculate location relative to window
        e.mouse_move = { loc.x(), loc.y() };

        LockGuard lock_guard(Window::focus_lock());
        if (!Window::is_any_focused())
            return;

        Window::focused().handle_event(e);
    }

    if (packet.wheel_delta) {
#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: mouse scroll delta " << packet.wheel_delta << " direction "
              << (packet.scroll_direction == ScrollDirection::VERTICAL ? "vertical" : "horizontal");
#endif
        LockGuard lock_guard(Window::focus_lock());
        if (!Window::is_any_focused())
            return;

        Event e {};

        e.type         = Event::Type::MOUSE_MOVE;
        e.mouse_scroll = { packet.scroll_direction, packet.wheel_delta };

        Window::focused().handle_event(e);
    }

    if (packet.left_button_state != m_last_mouse_state.left_button_state) {
        generate_button_state_event(VK::MOUSE_LEFT_BUTTON, packet.left_button_state);
#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: left button is_press: " << (packet.left_button_state == VKState::PRESSED);
#endif
    }
    if (packet.middle_button_state != m_last_mouse_state.middle_button_state) {
        generate_button_state_event(VK::MOUSE_MIDDLE_BUTTON, packet.middle_button_state);
#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: middle button is_press: " << (packet.middle_button_state == VKState::PRESSED);
#endif
    }
    if (packet.right_button_state != m_last_mouse_state.right_button_state) {
        generate_button_state_event(VK::MOUSE_RIGHT_BUTTON, packet.right_button_state);

#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: right button is_press: " << (packet.right_button_state == VKState::PRESSED);
#endif
    }
    if (packet.button_4_state != m_last_mouse_state.button_4_state) {
        generate_button_state_event(VK::MOUSE_BUTTON_4, packet.button_4_state);

#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: button 4 is_press: " << (packet.button_4_state == VKState::PRESSED);
#endif
    }
    if (packet.button_5_state != m_last_mouse_state.button_5_state) {
        generate_button_state_event(VK::MOUSE_BUTTON_5, packet.button_5_state);

#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: button 5 is_press: " << (packet.button_5_state == VKState::PRESSED);
#endif
    }

    m_last_mouse_state = packet;
}
}
