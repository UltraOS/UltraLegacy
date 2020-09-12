#include "WindowManager.h"
#include "Compositor.h"
#include "Multitasking/Process.h"
#include "Screen.h"

namespace kernel {

WindowManager WindowManager::s_instance;

void WindowManager::initialize()
{
    Screen::initialize();
    Process::create_supervisor(&Compositor::run);
}
}
