#include "Keyboard.h"
#include "PS2Keyboard.h"

namespace kernel {

Keyboard* Keyboard::s_device;

void Keyboard::discover_and_setup()
{
    // TODO: Look for HID keyboards

    s_device = new PS2Keyboard();
}
}
