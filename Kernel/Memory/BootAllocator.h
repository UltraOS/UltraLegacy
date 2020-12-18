#pragma once

#include "Common/Macros.h"
#include "Common/String.h"
#include "Common/Types.h"
#include "MemoryMap.h"

namespace kernel {

class BootAllocator {
    MAKE_SINGLETON(BootAllocator, MemoryMap);

public:
    // each allocation must be given a tag for easier tracking
    enum class Tag {
        GENERIC,
        KERNEL_IMAGE,
        KERNEL_MODULE,
        HEAP_BLOCK,
    };

    static constexpr StringView tag_to_string(Tag tag)
    {
        switch (tag) {
        case Tag::GENERIC:
            return "generic"_sv;
        case Tag::KERNEL_IMAGE:
            return "kernel image"_sv;
        case Tag::KERNEL_MODULE:
            return "kernel module"_sv;
        case Tag::HEAP_BLOCK:
            return "heap block"_sv;
        default:
            return "<unknown>"_sv;
        }
    }

    static void initialize(MemoryMap);
    static BootAllocator& the();

    Address64 reserve_at(Address64, size_t page_count, Tag = Tag::GENERIC);
    Address64 reserve_contiguous(size_t page_count, Address64 lower_limit, Address64 upper_limit, Tag = Tag::GENERIC);

    MemoryMap release();

private:
    constexpr MemoryMap::PhysicalRange::Type native_type_from_tag(Tag tag) const
    {
        using NativeType = MemoryMap::PhysicalRange::Type;

        switch (tag) {
        case Tag::GENERIC:
            return NativeType::GENERIC_BOOTALLOC_RESERVED;
        case Tag::KERNEL_IMAGE:
            return NativeType::KERNEL_IMAGE;
        case Tag::KERNEL_MODULE:
            return NativeType::KERNEL_MODULE;
        case Tag::HEAP_BLOCK:
            return NativeType::INITIAL_HEAP_BLOCK;
        default:
            ASSERT_NEVER_REACHED();
        }
    }

private:
    bool m_did_release { false };
    MemoryMap m_memory_map;
};

}