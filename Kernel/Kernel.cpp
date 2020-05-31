#include "Core/Types.h"
#include "Core/Logger.h"
#include "Interrupts/IDT.h"
#include "Interrupts/ISR.h"
#include "Interrupts/IRQManager.h"
#include "Interrupts/PIC.h"
#include "Interrupts/PIT.h"

using global_constructor_t = void(*)();
global_constructor_t global_constructors_begin;
global_constructor_t global_constructors_end;

namespace kernel {
    void run()
    {
        for (global_constructor_t* ctor = &global_constructors_begin; ctor < &global_constructors_end; ++ctor)
            (*ctor)();

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
