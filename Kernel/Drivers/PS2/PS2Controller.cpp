#include "PS2Controller.h"
#include "PS2Keyboard.h"

namespace kernel {

PS2Controller* PS2Controller::s_instance;

void PS2Controller::initialize()
{
    s_instance = new PS2Controller();
}

PS2Controller::PS2Controller()
{
    // initialize...
    // discover all devices...
    m_devices.emplace(new PS2Keyboard());
}
}
