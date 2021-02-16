#pragma once

#include "IDT.h"
#include "Memory/Range.h"
#include "Utilities.h"

namespace kernel {

class InterruptHandler;

class InterruptManager {
    MAKE_STATIC(InterruptManager);

public:
    static void initialize_all();

    static void register_handler(InterruptHandler&);
    static void unregister_handler(InterruptHandler&);

private:
    static void set_handler_for_vector(u16 vector, InterruptHandler&, bool user_callable);
    static void remove_handler_for_vector(u16 vector, InterruptHandler& owner);

    static void handle_interrupt(RegisterState*) USED;

private:
    static InterruptHandler* s_handlers[IDT::entry_count];
};

}
