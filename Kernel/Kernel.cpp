#include "Core/Types.h"
#include "Core/Logger.h"
#include "Interrupts/InterruptDescriptorTable.h"
#include "Interrupts/InterruptServiceRoutines.h"
#include "Interrupts/InterruptRequestManager.h"

using global_constructor_t = void(*)();
global_constructor_t global_constructors_begin;
global_constructor_t global_constructors_end;

namespace kernel {
    void run()
    {
        for (global_constructor_t* ctor = &global_constructors_begin; ctor < &global_constructors_end; ++ctor)
            (*ctor)();

        cli();
        InterruptServiceRoutines::install();
        InterruptRequestManager::the().install();
        InterruptDescriptorTable::the().install();
        sti();

        log() << "Hello from the kernel!";

        for(;;);
    }
}
