#include "MemoryMap.h"

#include "Core/Boot.h"
#include "Page.h"

// #define MEMORY_MAP_DEBUG

namespace kernel {

static constexpr size_t memory_map_entries_buffer_size = Page::size;
extern "C" u8 memory_map_entries_buffer[memory_map_entries_buffer_size];

void MemoryMap::copy_aligned_from(E820MemoryMap& e820)
{
    for (auto& dirty_entry : e820) {
#ifdef MEMORY_MAP_DEBUG
        log() << "MemoryMap: Considering entry at " << format::as_hex
              << dirty_entry.base_address << " size: " << dirty_entry.length;
#endif

        PhysicalRange new_range(
            dirty_entry.base_address,
            dirty_entry.length,
            static_cast<PhysicalRange::Type>(dirty_entry.type));

        // don't try to align reserved physical ranges, we're not gonna use them anyways
        if (new_range.type != PhysicalRange::Type::FREE) {
            emplace_range(move(new_range));
            continue;
        }

        new_range = new_range.aligned_to(Page::size);

        if (new_range.empty() || new_range.length() < Page::size) {
#ifdef MEMORY_MAP_DEBUG
            log() << "MemoryMap: Skipped physical range " << format::as_hex
                  << dirty_entry.base_address << " of size " << dirty_entry.length;
#endif
            continue;
        }

        new_range.set_length(Page::round_down(new_range.length()));

        emplace_range(move(new_range));
    }
}

void MemoryMap::sort_by_address()
{
    // 99% of BIOSes return a sorted memory map, which insertion sort handles at O(N)
    // whereas quicksort would run at O(N^2). Even if it's unsorted, most E820 memory maps
    // only contain like 10-20 entries, so it's not a big deal.
    insertion_sort(begin(), end());
}

// Splits/merges two overlapping physical ranges while taking their types into account
StaticArray<MemoryMap::PhysicalRange, 3>
MemoryMap::PhysicalRange::shatter_against(const PhysicalRange& other) const
{
    StaticArray<PhysicalRange, 3> ranges {};

    // cannot shatter against non-overlapping range
    ASSERT(contains(other.begin()));

    // cut out the overlapping piece by default
    ranges[0] = *this;
    ranges[0].set_end(other.begin());

    // both ranges have the same type, so we can just merge them
    if (type == other.type) {
        ranges[0].set_end(end() > other.end() ? end() : other.end());
        return ranges;
    }

    // other range is fully inside this range
    if (other.end() <= end()) {
        ranges[2].reset_with_two_pointers(other.end(), end());
        ranges[2].type = type;
    }

    // we cut out the overlapping piece of the other range and make it our type
    if (type > other.type) {
        ranges[0].set_end(end());

        if (end() <= other.end()) {
            ranges[1] = other;
            ranges[1].set_begin(ranges[0].end());
        } else { // since we swallowed the other range we don't need this
            ranges[2] = {};
        }
    } else { // our overlapping piece gets cut out and put into the other range
        ranges[1] = other;
    }

    return ranges;
}

void MemoryMap::erase_range_at(size_t index)
{
    ASSERT(index < m_entry_count);

    if (index == m_entry_count - 1) {
        --m_entry_count;
        return;
    }

    size_t bytes_to_move = (m_entry_count - index - 1) * sizeof(PhysicalRange);
    move_memory(&at(index + 1), &at(index), bytes_to_move);

    --m_entry_count;
}

// This function assumes the ranges are sorted in ascending order
void MemoryMap::correct_overlapping_ranges(size_t hint)
{
    ASSERT(m_entry_count != 0);

    auto trivially_mergeable = [](const PhysicalRange& l, const PhysicalRange& r) {
        return l.end() == r.begin() && l.type == r.type;
    };

    for (size_t i = hint; i < m_entry_count - 1; ++i) {
        while (i < m_entry_count - 1 && (m_entries[i].overlaps(m_entries[i + 1]) || trivially_mergeable(m_entries[i], m_entries[i + 1]))) {
            if (trivially_mergeable(m_entries[i], m_entries[i + 1])) {
                m_entries[i].set_end(m_entries[i + 1].end());
                erase_range_at(i + 1);
                continue;
            }

            auto new_ranges = m_entries[i].shatter_against(m_entries[i + 1]);

            bool is_valid_range[3] {};
            for (size_t j = 0; j < 3; ++j) {
                if (new_ranges[j].is_free()) {
                    new_ranges[j] = new_ranges[j].aligned_to(Page::size);
                    new_ranges[j].set_length(Page::round_down(new_ranges[j].length()));
                }

                is_valid_range[j] = new_ranges[j] && (new_ranges[j].length() >= Page::size || new_ranges[j].is_reserved());
            }

            auto j = i;

            for (size_t k = 0; k < 3; ++k) {
                if (!is_valid_range[k])
                    continue;

                if (j - i == 2) {
                    emplace_range_at(j++, move(new_ranges[k]));
                    break;
                }

                m_entries[j++] = move(new_ranges[k]);
            }

            if (j == i) {
                StackStringBuilder error_message;
                error_message << "MemoryMap: error while correcting ranges, couldn't merge:\n"
                              << m_entries[i] << "\nwith\n"
                              << m_entries[i + 1];
                runtime::panic(error_message.data());
            }

            // only 1 range ended up being valid, erase the second
            if (j - i == 1) {
                erase_range_at(j);
            } // else we emplaced 2 or more ranges

            // walk backwards one step, because the type of range[i] could've changed
            // therefore there's a small chance we might be able to merge i and i - 1
            if (i != 0)
                --i;
        }
    }
}

MemoryMap MemoryMap::generate_from(LoaderContext* context)
{
    ASSERT(context->type == LoaderContext::Type::BIOS);

    auto& raw_memory_map = static_cast<BIOSContext*>(context)->memory_map;

    MemoryMap clean_map {};
    clean_map.set_range_buffer(memory_map_entries_buffer, memory_map_entries_buffer_size);
    clean_map.copy_aligned_from(raw_memory_map);
    clean_map.sort_by_address();
    clean_map.correct_overlapping_ranges();

    return clean_map;
}

}