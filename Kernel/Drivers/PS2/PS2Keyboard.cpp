#include "PS2Keyboard.h"
#include "Common/Logger.h"

namespace kernel {

PS2Keyboard::PS2Keyboard(PS2Controller::Channel channel) : PS2Device(channel)
{
    enable_irq();
}

void PS2Keyboard::handle_action()
{
    log() << "Keyboard: Got a key press";
}
}
