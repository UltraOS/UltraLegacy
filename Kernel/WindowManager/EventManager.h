#pragma once

#include "Event.h"
#include "VirtualKey.h"

#include "Drivers/Keyboard.h"
#include "Drivers/Mouse.h"

namespace kernel {

class EventManager {
    MAKE_SINGLETON(EventManager) = default;

public:
    static EventManager& the() { return s_instance; }

    void post_action(const Keyboard::Packet&);
    void post_action(const Mouse::Packet&);

private:
    void update_state_of_key(VK, VKState);
    void generate_button_state_event(VK, VKState);

private:
    Mouse::Packet m_last_mouse_state {};

    VKState m_key_state[static_cast<u8>(VK::LAST)] { VKState::RELEASED };

    static EventManager s_instance;
};
}
