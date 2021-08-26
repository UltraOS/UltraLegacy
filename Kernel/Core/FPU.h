#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class FPU {
    MAKE_STATIC(FPU)
public:
    static void detect_features();
    static void initialize_for_this_cpu();

    struct Features {
        bool sse1;
        bool sse2;
        bool sse3;
        bool ssse3;
        bool sse4_1;
        bool sse4_2;
        bool avx;
        bool avx2;
        bool avx512;
        bool fxsave;
        bool xsave;
        u32 xcr0_supported_bits;
        u32 mxcsr_mask;
        size_t save_area_bytes;
    };
    Features& features() { return s_features; }

    static void* allocate_state();
    static void free_state(void*);

    static void save_state(void*);
    static void restore_state(void*);

private:
    static Features s_features;
};

}
