#include "Common/Logger.h"
#include "FPU.h"
#include "CPU.h"

namespace kernel {

FPU::Features FPU::s_features;

void FPU::detect_features()
{
    auto id1 = CPU::ID(1);
    auto id7 = CPU::ID(7);

    String supported_features;
    supported_features.reserve(64);

    auto supported_if = [&supported_features](bool test, bool& out, StringView repr) {
        if (!test)
            return;

        out = true;
        supported_features += " "_sv;
        supported_features += repr;
    };

    supported_if(IS_BIT_SET(id1.d, 25), s_features.sse1, "SSE"_sv);
    supported_if(IS_BIT_SET(id1.d, 26), s_features.sse2, "SSE2"_sv);
    supported_if(IS_BIT_SET(id1.c, 0), s_features.sse3, "SSE3"_sv);
    supported_if(IS_BIT_SET(id1.c, 9), s_features.ssse3, "SSSE3"_sv);
    supported_if(IS_BIT_SET(id1.c, 19), s_features.sse4_1, "SSE4.1"_sv);
    supported_if(IS_BIT_SET(id1.c, 20), s_features.sse4_2, "SSE4.2"_sv);
    supported_if(IS_BIT_SET(id1.c, 28), s_features.avx, "AVX");
    supported_if(IS_BIT_SET(id1.d, 24), s_features.fxsave, "FXSAVE");
    supported_if(IS_BIT_SET(id1.c, 26), s_features.xsave, "XSAVE");
    supported_if(IS_BIT_SET(id7.d, 26), s_features.avx2, "AVX2");
    supported_if(IS_BIT_SET(id7.b, 16), s_features.avx512, "AVX512");

    if (supported_features.empty())
        supported_features << "NONE";

    log() << "FPU: supported extensions ->" << supported_features;

    if (s_features.xsave) {
        auto idd = CPU::ID(0xD);
        s_features.save_area_bytes = idd.c;
        s_features.xcr0_supported_bits = idd.a;
        log() << "XSAVE area bytes: " << s_features.save_area_bytes;
    } else if (s_features.fxsave) {
        s_features.save_area_bytes = 512;
    } else {
#ifdef ULTRA_64
        runtime::panic("No FXSAVE support for an AMD64 CPU");
#endif
        s_features.save_area_bytes = 104;
    }
}

void FPU::initialize_for_this_cpu()
{
    auto cr4 = CPU::read_cr4();

    if (s_features.sse1) {
        static constexpr size_t fxsave_awareness_bit = SET_BIT(9);
        static constexpr size_t simd_exceptions_support_bit = SET_BIT(10);

        cr4 |= fxsave_awareness_bit;
        cr4 |= simd_exceptions_support_bit;

        CPU::write_cr4(cr4);

        if (s_features.mxcsr_mask == 0) {
            alignas(16) u8 fxsave_region[512] {};
            asm volatile ("fxsave %0" : "=m"(*fxsave_region));

            // From the intel manual:
            // "Check the value in the MXCSR_MASK field in the FXSAVE image (bytes 28 through 31)."
            copy_memory(fxsave_region + 28, &s_features.mxcsr_mask, sizeof(u32));

            // From the intel manual:
            // "If the value of the MXCSR_MASK field is 00000000H, then the MXCSR_MASK
            // value is the default value of 0000FFBFH"
            if (s_features.mxcsr_mask == 0)
                s_features.mxcsr_mask = 0x0000FFBF;
        }
    }

    if (s_features.xsave) {
        static constexpr size_t osxsave_awareness_bit = SET_BIT(18);
        cr4 |= osxsave_awareness_bit;
        CPU::write_cr4(cr4);

        static constexpr size_t xcr0_x87_bit = SET_BIT(0);
        static constexpr size_t xcr0_sse_bit = SET_BIT(1);
        static constexpr size_t xcr0_avx_bit = SET_BIT(2);
        static constexpr size_t xcr0_opmask_bit = SET_BIT(5);
        static constexpr size_t xcr0_upper_half_of_lower_zmm_bit = SET_BIT(6);
        static constexpr size_t xcr0_upper_zmm_bit = SET_BIT(7);

        CPU::XCR xcr0 {};

        if (s_features.xcr0_supported_bits & xcr0_x87_bit)
            xcr0.lower |= xcr0_x87_bit;
        if (s_features.xcr0_supported_bits & xcr0_sse_bit)
            xcr0.lower |= xcr0_sse_bit;
        if (s_features.xcr0_supported_bits & xcr0_avx_bit)
            xcr0.lower |= xcr0_avx_bit;
        if (s_features.xcr0_supported_bits & xcr0_opmask_bit)
            xcr0.lower |= xcr0_opmask_bit;
        if (s_features.xcr0_supported_bits & xcr0_upper_half_of_lower_zmm_bit)
            xcr0.lower |= xcr0_upper_half_of_lower_zmm_bit;
        if (s_features.xcr0_supported_bits & xcr0_upper_zmm_bit)
            xcr0.lower |= xcr0_upper_zmm_bit;

        xcr0.write(0);
    }
}

void* FPU::allocate_state()
{
    ASSERT(s_features.save_area_bytes != 0);
    void* ptr = HeapAllocator::allocate(s_features.save_area_bytes, s_features.xsave ? 64 : 16);
    zero_memory(ptr, s_features.save_area_bytes);

    // From intel manual:
    // "When the x87 FPU is initialized with either an FINIT/FNINIT or FSAVE/FNSAVE instruction, the x87 FPU control
    // word is set to 037FH, which masks all floating-point exceptions, sets rounding to nearest, and sets the x87 FPU
    // precision to 64 bits."
    static constexpr u16 default_fcw_value = 0x037F;
    static constexpr size_t fcw_offset = 0;

    u8* byte_ptr = reinterpret_cast<u8*>(ptr);

    if (s_features.fxsave || s_features.xsave) {
        // Reserved set to 0, flush-to-zero and rounding control set to default,
        // exceptions masked, no flags set.
        static constexpr u32 default_mxcsr_value = 0b0001111111000000;
        static constexpr size_t mxcsr_offset = 24;

        u32 mxcsr_to_write = default_mxcsr_value & s_features.mxcsr_mask;

        *Address(byte_ptr + fcw_offset).as_pointer<u16>() = default_fcw_value;
        *Address(byte_ptr + mxcsr_offset).as_pointer<u32>() = mxcsr_to_write;
        // we don't set tag word because it's stored in a compressed way where 0 means empty
    } else {
        // From intel manual:
        // "When the x87 FPU is initialized with either an FINIT/FNINIT or FSAVE/FNSAVE instruction,
        // the x87 FPU tag word is set to FFFFH, which marks all the x87 FPU data registers as empty."
        static constexpr u16 default_tag_value = 0xFFFF;
        static constexpr size_t tag_offset = 8;

        *Address(byte_ptr + fcw_offset).as_pointer<u16>() = default_fcw_value;
        *Address(byte_ptr + tag_offset).as_pointer<u16>() = default_tag_value;
    }

    return ptr;
}

void FPU::free_state(void* ptr)
{
    HeapAllocator::free(ptr);
}

void FPU::save_state(void* ptr)
{
    u8* byte_ptr = reinterpret_cast<u8*>(ptr);

    if (s_features.xsave) {
        u32 low = 0xFFFFFFFF;
        u32 high = 0xFFFFFFFF;

        asm volatile("xsave %0" : "=m"(*byte_ptr) : "a"(low), "d"(high) : "memory");
    } else if (s_features.fxsave) {
        asm volatile ("fxsave %0" : "=m"(*byte_ptr));
    } else {
        asm volatile ("fnsave %0" : "=m"(*byte_ptr));
    }
}

void FPU::restore_state(void* ptr)
{
    u8* byte_ptr = reinterpret_cast<u8*>(ptr);

    if (s_features.xsave) {
        u32 low = 0xFFFFFFFF;
        u32 high = 0xFFFFFFFF;

        asm volatile("xrstor %0" :: "m"(*byte_ptr), "a"(low), "d"(high));
    } else if (s_features.fxsave) {
        asm volatile ("fxrstor %0" :: "m"(*byte_ptr));
    } else {
        asm volatile ("frstor %0" :: "m"(*byte_ptr));
    }
}

}
