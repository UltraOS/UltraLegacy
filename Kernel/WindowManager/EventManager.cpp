#include "EventManager.h"

namespace kernel {

EventManager EventManager::s_instance;

void EventManager::post_action(const Keyboard::Packet&)
{
    // do something
}

void EventManager::post_action(const Mouse::Packet&)
{
    // do something
}
}
