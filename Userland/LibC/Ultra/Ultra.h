#pragma once

#include "Memory.h"
#include "Process.h"
#include "IO.h"
#include "VirtualKey.h"
#include "Event.h"
#include "Error.h"
#include "Window.h"

void debug_log(const char* message);
unsigned long ticks_since_boot();
