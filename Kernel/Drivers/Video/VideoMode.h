#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel {

struct PACKED VBEVideoMode {
    u32 width;
    u32 height;
    u32 pitch;
    u32 bpp;
    ptr_t framebuffer;
};

// Native video mode == VBEVideoMode for now
struct VideoMode : VBEVideoMode {
};
}