#pragma once

#include <Shared/VirtualKey.h>

enum VK {
#define VIRTUAL_KEY(key, representation) VK_ ## key,
    ENUMERATE_VIRTUAL_KEYS
#undef VIRTUAL_KEY
};

enum VKState {
#define VK_STATE(state) VKS_ ## state,
    ENUMERATE_VK_STATES
#undef VK_STATE
};
