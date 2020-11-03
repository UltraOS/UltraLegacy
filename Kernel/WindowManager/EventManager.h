#pragma once

#include "Common/DynamicArray.h"
#include "Common/Lock.h"

#include "Event.h"
#include "VirtualKey.h"

#include "Drivers/Keyboard.h"
#include "Drivers/Mouse.h"

namespace kernel {

class EventManager {
    static constexpr size_t max_event_queue_size = 64;

    MAKE_SINGLETON(EventManager)
        : m_event_queue(max_event_queue_size)
    {
    }

public:
    static EventManager& the() { return s_instance; }

    void post_action(const Keyboard::Packet&);
    void post_action(const Mouse::Packet&);
    void push_event(const Event&);

    void dispatch_pending();

    VKState state_of_key(VK key)
    {
        ASSERT(static_cast<u8>(key) < static_cast<u8>(VK::LAST));

        return m_key_state[static_cast<u8>(key)];
    }

private:
    void update_state_of_key(VK, VKState);
    void generate_button_state_event(VK, VKState);
    void generate_char_typed_if_applicable(VK);

private:
    InterruptSafeSpinLock m_event_queue_lock;

    DynamicArray<Event> m_event_queue;

    Mouse::Packet m_last_mouse_state {};

    VKState m_key_state[static_cast<u8>(VK::LAST)] { VKState::RELEASED };

    static EventManager s_instance;
};
}
