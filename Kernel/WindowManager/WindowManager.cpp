#include "WindowManager.h"
#include "Compositor.h"
#include "Multitasking/Process.h"
#include "Screen.h"

namespace kernel {

WindowManager WindowManager::s_instance;

void WindowManager::initialize()
{
    Screen::initialize(VideoDevice::the());
    the().m_desktop_window = Window::create_desktop(*Thread::current(), Screen::the().rect());
    Process::create_supervisor(&Compositor::run);
}
}
