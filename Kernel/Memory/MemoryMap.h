#pragma once

#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Common/Types.h"
#include "Common/Pair.h"
#include "Common/String.h"
#include "Common/StaticArray.h"
#include "Range.h"

namespace kernel {

struct PACKED E820MemoryMap {
    struct PACKED PhysicalRange {
        u64 base_address;
        u64 length;

        enum class Type : u32 {
            FREE = 1,
            RESERVED = 2,
            RECLAIMABLE = 3,
            NON_VOLATILE = 4,
            BAD = 5
        } type;

        u32 extended_attributes;
    };

    PhysicalRange* entries;
    u32 entry_count;

    PhysicalRange* begin() const { return entries; }
    PhysicalRange* end() const { return entries + entry_count; }
};

class LoaderContext;

// Native UltraOS memory map format:
// PhysicalRange entries are all unique and are sorted in ascending order
struct MemoryMap {
    struct PhysicalRange : LongRange {
        enum class Type : u64 {
            FREE = 1,
            RESERVED = 2,
            RECLAIMABLE = 3,
            NON_VOLATILE = 4,
            BAD = 5,
            KERNEL_IMAGE = static_cast<uint64_t>(UINT32_MAX) + 1, // make sure we never overlap with E820 types
            KERNEL_MODULE,
            INITIAL_HEAP_BLOCK,
            GENERIC_BOOTALLOC_RESERVED,
        } type { Type::RESERVED };

        friend bool operator>(Type left, Type right)
        {
            return static_cast<u64>(left) > static_cast<u64>(right);
        }

        PhysicalRange() = default;

        PhysicalRange(Address64 start, size_t length, Type type = Type::RESERVED)
            : LongRange(start, length), type(type)
        {}

        bool is_free() const
        {
            return type == Type::FREE;
        }

        bool is_reserved() const
        {
            return !is_free();
        }

        StringView type_as_string() const {
            switch (type) {
                case Type::FREE:
                    return "free"_sv;
                case Type::RESERVED:
                    return "reserved"_sv;
                case Type::RECLAIMABLE:
                    return "reclaimable"_sv;
                case Type::NON_VOLATILE:
                    return "ACPI non-volatile"_sv;
                case Type::BAD:
                    return "bad"_sv;
                case Type::KERNEL_IMAGE:
                    return "kernel image"_sv;
                case Type::KERNEL_MODULE:
                    return "kernel module"_sv;
                case Type::INITIAL_HEAP_BLOCK:
                    return "initial heap block"_sv;
                case Type::GENERIC_BOOTALLOC_RESERVED:
                    return "generic boot allocator reserved";
                default:
                    return "unknown"_sv;
            }
        }

        bool operator<(const PhysicalRange& other) const
        {
            return LongRange::operator<(static_cast<const BasicRange&>(other));
        }

        bool operator==(const PhysicalRange& other) const
        {
            return LongRange::operator==(other) && type == other.type;
        }

        bool operator!=(const PhysicalRange& other) const
        {
            return !operator==(other);
        }

        PhysicalRange aligned_to(size_t alignment) const
        {
            PhysicalRange new_range {};
            new_range.reset_with(BasicRange::aligned_to(alignment));
            new_range.type = type;

            return new_range;
        }

        static PhysicalRange create_empty_at(Address64 address)
        {
            auto range = BasicRange::create_empty_at(address);

            PhysicalRange phys_range {};
            phys_range.reset_with(range);

            return phys_range;
        }

        template<typename LoggerT>
        friend LoggerT& operator<<(LoggerT&& logger, const PhysicalRange& range) {
            logger << "PhysicalRange: start:" << format::as_hex << range.begin() << " size:" << range.length()
                   << " type: " << range.type_as_string();

            return logger;
        }

        StaticArray<PhysicalRange, 3> shatter_against(const PhysicalRange& other) const;
    };

    static MemoryMap generate_from(LoaderContext*);

    Address64 highest_address() const
    {
        ASSERT(m_entry_count > 0);

        // since entries are sorted in ascending order we can afford to do this
        return m_entries[m_entry_count - 1].end();
    }
    
    PhysicalRange* begin() const { return m_entries; }
    PhysicalRange* end() const { return m_entries + m_entry_count; }

    size_t entry_count() const { return m_entry_count; }

    PhysicalRange& at(size_t index)
    {
        ASSERT(index < m_entry_count);

        return m_entries[index];
    }

    template <typename... Args>
    void emplace_range(Args&&... args)
    {
        if (m_entry_count >= m_capacity)
            warning() << "MemoryMap: Expanding past reserved capacity (" << m_capacity << ") entry count -> " << m_entry_count;

        new (end()) PhysicalRange(forward<Args>(args)...);
        ++m_entry_count;
    }

    template <typename... Args>
    void emplace_range_at(size_t index, Args&&... args)
    {
        ASSERT(index < m_entry_count);

        if (m_entry_count >= m_capacity)
            warning() << "MemoryMap: Expanding past reserved capacity (" << m_capacity << ") entry count -> " << m_entry_count;

        size_t bytes_to_move = (m_entry_count - index) * sizeof(PhysicalRange);
        ++m_entry_count;
        move_memory(&at(index), &at(index + 1), bytes_to_move);

        at(index) = PhysicalRange(forward<Args>(args)...);
    }
    
    void erase_range_at(size_t index);

    void set_range_buffer(u8* buffer, size_t size)
    {
        m_entries = reinterpret_cast<PhysicalRange*>(buffer);
        m_capacity = size / sizeof(PhysicalRange);
        m_entry_count = 0;
    }

    void correct_overlapping_ranges(size_t hint = 0);

private:
    void copy_aligned_from(E820MemoryMap&);
    void sort_by_address();

private:
    PhysicalRange* m_entries { nullptr };
    size_t m_capacity { 0 };
    size_t m_entry_count { 0 };
};
}
