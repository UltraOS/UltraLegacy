#pragma once

#include "Common/DynamicArray.h"
#include "Window.h"

namespace kernel {

class WindowManager {
public:
    static void initialize();

private:
    DynamicArray<RefPtr<Window>> m_windows;

    static WindowManager s_instance;
};

}
