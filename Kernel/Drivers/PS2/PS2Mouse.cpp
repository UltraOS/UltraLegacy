#include "PS2Mouse.h"
#include "Common/Logger.h"

namespace kernel {

PS2Mouse::PS2Mouse(PS2Controller::Channel channel) : PS2Device(channel)
{
    // detect the mouse wheel/5 buttons here
    // set proper config

    send_command(0xF6);
    enable_irq();
}

void PS2Mouse::handle_action()
{
    while (PS2Controller::the().status().output_full) {
        log() << "MouseMoved: " << format::as_hex << read_data();
    }
}
}
