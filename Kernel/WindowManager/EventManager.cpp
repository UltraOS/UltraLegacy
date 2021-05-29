#include "EventManager.h"
#include "Common/Logger.h"
#include "Screen.h"
#include "Window.h"
#include "WindowManager.h"

// #define EVENT_MANAGER_DEBUG

namespace kernel {

EventManager EventManager::s_instance;

void EventManager::dispatch_pending()
{
    for (;;) {
        auto event = pop_event();

        switch (event.type) {
        case EventType::EMPTY:
            return;
        case EventType::BUTTON_STATE:
            update_state_of_key(event.vk_state.vkey, event.vk_state.state);
            if (event.vk_state.state == VKState::PRESSED)
                Screen::the().check_if_focused_window_should_change();
            if (!Window::is_any_focused())
                continue;
            Window::focused().handle_event(event);
            continue;

        case EventType::MOUSE_MOVE: {
            auto& mouse_event = event.mouse_move;
            Screen::the().move_cursor_to({ static_cast<ssize_t>(mouse_event.x), static_cast<ssize_t>(mouse_event.y) });

            bool event_handled = false;
            for (auto& window : WindowManager::the().windows()) {
                event_handled |= window.handle_event(event, event_handled);
            }
            continue;
        }

        case EventType::MOUSE_SCROLL:
            for (auto& window : WindowManager::the().windows()) {
                if (window.full_translated_rect().contains(Screen::the().cursor().location())) {
                    window.handle_event(event, false);
                    break;
                }
            }
            continue;

        case EventType::KEY_STATE:
            update_state_of_key(event.vk_state.vkey, event.vk_state.state);
            [[fallthrough]];
        case EventType::CHAR_TYPED:
            if (!Window::is_any_focused())
                continue;

            Window::focused().handle_event(event);
            continue;

        case EventType::WINDOW_RESIZE:
        case EventType::WINDOW_MOVE:
            continue;

        default:
            ASSERT_NEVER_REACHED();
        }
    }
}

void EventManager::push_event(const Event& event)
{
    LOCK_GUARD(m_event_queue_lock);
    m_event_queue.enqueue(event);
}

Event EventManager::pop_event()
{
    LOCK_GUARD(m_event_queue_lock);

    if (m_event_queue.empty()) {
        Event e {};
        e.type = EventType::EMPTY;
        return e;
    }

    return m_event_queue.dequeue();
}

void EventManager::update_state_of_key(VK key, VKState state)
{
    ASSERT(static_cast<u8>(key) < static_cast<u8>(VK::LAST));

    // keys that must be toggled
    if (key == VK::CAPS_LOCK || key == VK::NUM_LK || key == VK::SCR_LK) {
        if (state == VKState::RELEASED)
            return;

        auto& state_of_key = m_key_state[static_cast<u8>(key)];

        state_of_key = (state_of_key == VKState::PRESSED) ? VKState::RELEASED : VKState::PRESSED;
        return;
    }

    m_key_state[static_cast<u8>(key)] = state;
}

void EventManager::generate_button_state_event(VK key, VKState state)
{
    Event e {};
    e.type = EventType::BUTTON_STATE;
    e.vk_state = { key, state };

    push_event(e);
}

void EventManager::generate_char_typed_if_applicable(VK key)
{
    bool is_convertable = false;
    auto as_char = to_char(key, state_of_key(VK::LEFT_SHIFT), state_of_key(VK::CAPS_LOCK), is_convertable);

    if (is_convertable) {
        Event e {};
        e.type = EventType::CHAR_TYPED;
        e.char_typed.character = static_cast<size_t>(as_char);
        push_event(e);
    }
}

void EventManager::post_action(const Keyboard::Packet& packet)
{
#ifdef EVENT_MANAGER_DEBUG
    log() << "EventManager: key press " << to_string(packet.key) << " is_press: " << (packet.state == VKState::PRESSED);
#endif

    Event e {};
    e.type = EventType::KEY_STATE;
    e.vk_state = { packet.key, packet.state };

    push_event(e);

    if (packet.state == VKState::PRESSED)
        generate_char_typed_if_applicable(packet.key);
}

void EventManager::post_action(const Mouse::Packet& packet)
{
    // detect changes of state
    if (packet.x_delta || packet.y_delta) {
#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: mouse move x " << packet.x_delta << " y " << packet.y_delta;
#endif
        Event e {};
        e.type = EventType::MOUSE_MOVE;
        auto loc = Screen::the().cursor_position_for_delta(packet.x_delta, packet.y_delta);
        e.mouse_move = { static_cast<size_t>(loc.x()), static_cast<size_t>(loc.y()) };

        push_event(e);
    }

    if (packet.wheel_delta) {
#ifdef EVENT_MANAGER_DEBUG
        log() << "EventManager: mouse scroll delta " << packet.wheel_delta << " direction "
              << (packet.scroll_direction == ScrollDirection::VERTICAL ? "vertical" : "horizontal");
#endif
        Event e {};

        e.type = EventType::MOUSE_SCROLL;
        e.mouse_scroll = { packet.scroll_direction, packet.wheel_delta };

        push_event(e);
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
