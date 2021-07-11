#pragma once


#include <Shared/WMCommands.h>

enum WMCommand {
#define WM_COMMAND(name) WM_## name,
ENUMERATE_WM_COMMANDS
#undef WM_COMMAND
};

#include <Shared/Window.h>

void execute_window_command(void*);