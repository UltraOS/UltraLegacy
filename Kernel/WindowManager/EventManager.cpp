#include "EventManager.h"
#include "Common/Logger.h"

namespace kernel {

EventManager EventManager::s_instance;

void EventManager::post_action(const Keyboard::Packet& packet)
{
    log() << "KeyPress: " << to_string(packet.key) << " is_press: " << (packet.state == Keyboard::Packet::State::PRESS);
}

void EventManager::post_action(const Mouse::Packet& packet)
{
    // detect changes of state
    if (packet.x_delta || packet.y_delta) {
        log() << "MouseMove: x " << packet.x_delta << " y " << packet.y_delta;
    }

    if (packet.wheel_delta) {
        log() << "MouseScroll: delta " << packet.wheel_delta << " direction "
              << (packet.scroll_direction == Mouse::Packet::ScrollDirection::VERTICAL ? "vertical" : "horizontal");
    }

    if (packet.left_button_held != m_last_mouse_state.left_button_held) {
        log() << "ButtonPress: left is_press: " << packet.left_button_held;
    }
    if (packet.middle_button_held != m_last_mouse_state.middle_button_held) {
        log() << "ButtonPress: middle is_press: " << packet.middle_button_held;
    }
    if (packet.right_button_held != m_last_mouse_state.right_button_held) {
        log() << "ButtonPress: right is_press: " << packet.right_button_held;
    }
    if (packet.button_4_held != m_last_mouse_state.button_4_held) {
        log() << "ButtonPress: 4 is_press: " << packet.button_4_held;
    }
    if (packet.button_5_held != m_last_mouse_state.button_5_held) {
        log() << "ButtonPress: 5 is_press: " << packet.button_5_held;
    }

    m_last_mouse_state = packet;
}
}
