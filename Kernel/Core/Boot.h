#pragma once

#include "Memory/MemoryMap.h"
#include "Drivers/Video/VideoMode.h"

namespace kernel {

struct PACKED LoaderContext {
    enum class Type : ptr_t {
        BIOS = 1,
    } type;
};

struct PACKED BIOSContext : public LoaderContext {
    VBEVideoMode video_mode;
    E820MemoryMap memory_map;
};

}
