#pragma once

#include "Memory/PhysicalMemory.h"

namespace kernel {

struct PACKED VideoMode {
    u32   width;
    u32   height;
    u32   pitch;
    u32   bpp;
    ptr_t framebuffer;
};

struct Context {
    VideoMode video_mode;
    MemoryMap memory_map;
};
}
