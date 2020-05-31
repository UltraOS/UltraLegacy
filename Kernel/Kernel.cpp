#include "Core/Types.h"
#include "Core/Logger.h"
#include "Core/Runtime.h"
#include "Interrupts/IDT.h"
#include "Interrupts/ISR.h"
#include "Interrupts/IRQManager.h"
#include "Interrupts/PIC.h"
#include "Interrupts/PIT.h"

namespace kernel {

    void run()
    {
        runtime::init_global_objects();

        cli();
        PIT timer;
        ISR::install();
        IRQManager::the().install();
        IDT::the().install();
        sti();

        log() << "Hello from the kernel!";

        for(;;);
    }
}
