#pragma once

#include "Common/RefPtr.h"
#include "VirtualKey.h"

#include "Drivers/Keyboard.h"
#include "Drivers/Mouse.h"

namespace kernel {

class EventManager {
public:
    EventManager& the() { return s_instance; }

    void post_action(const Keyboard::Packet&);
    void post_action(const Mouse::Packet&);

private:
    static EventManager s_instance;
};
}
