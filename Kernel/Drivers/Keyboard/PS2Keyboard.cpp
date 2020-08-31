#include "PS2Keyboard.h"
#include "Common/Logger.h"

namespace kernel {

PS2Keyboard::PS2Keyboard() : IRQHandler(irq_number)
{
    enable_irq();
}

void PS2Keyboard::handle_irq(const RegisterState&)
{
    log() << "Keyboard: Got a key press";
}
}
