#pragma once

#include "Core/Runtime.h"

namespace kernel {

class Compositor {
public:
    static void run();

private:
    static void redraw_clock_widget();
};
}
