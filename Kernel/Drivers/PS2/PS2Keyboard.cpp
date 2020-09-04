#include "PS2Keyboard.h"
#include "Common/Logger.h"

namespace kernel {

PS2Keyboard::PS2Keyboard(PS2Controller::Channel channel) : PS2Device(channel)
{
    enable_irq();
}

void PS2Keyboard::handle_action()
{
    while (PS2Controller::the().status().output_full) {
        log() << "Key pressed: " << format::as_hex << read_data();
    }
}
}
