#pragma once

#include "DynamicArray.h"
#include "Macros.h"
#include "Math.h"
#include "Optional.h"
#include "Pair.h"

namespace kernel {

class DynamicBitArray {
public:
    using storage_unit_type = ptr_t;
    static constexpr size_t bits_per_unit = sizeof(storage_unit_type) * 8;

    DynamicBitArray() = default;

    explicit DynamicBitArray(size_t initial_size)
    {
        set_size(initial_size);
    }

    size_t size() const { return m_bit_count; }

    // checks sizeof(void*) * 8 bits at a time
    Optional<size_t> find_bit(bool of_value, size_t hint = 0) const
    {
        ASSERT(hint < m_bit_count);

        auto start = hint / bits_per_unit;
        auto end = ceiling_divide(m_bit_count, bits_per_unit);

        // TODO: numeric_limits<storage_unit_type>::max();
        auto bad_mask = of_value ? 0 : static_cast<storage_unit_type>(-1);

        for (size_t pass = 0; pass < 2; ++pass) {
            for (size_t index_of_unit = start; index_of_unit < end; ++index_of_unit) {
                if (m_bits[index_of_unit] == bad_mask)
                    continue;

                auto unit_value = of_value ? m_bits[index_of_unit] : ~m_bits[index_of_unit];
                auto target_bit = __builtin_ffsl(unit_value) - 1 + index_of_unit * bits_per_unit;

                // looks like the bit we found is outside of our size
                if (target_bit >= m_bit_count)
                    break;

                return target_bit;
            }

            // hint failed so reset start to zero and do one more pass
            end = start;
            start = 0;
        }

        return {};
    }

    Optional<size_t> find_bit_starting_at_offset(bool of_value, size_t offset) const
    {
        ASSERT(offset < m_bit_count);

        for (size_t index = offset; index < m_bit_count; ++index) {
            auto loc = location_of_bit(index);

            if ((m_bits[loc.first] & SET_BIT(loc.second)) == of_value)
                return index;
        }

        return {};
    }

    Optional<size_t> find_range(size_t length, bool of_value) const
    {
        size_t contiguous_bits = 0;
        ssize_t first_bit = -1;

        for (size_t i = 0; i < m_bit_count; ++i) {
            auto is_suitable = of_value == bit_at(i);

            if (is_suitable) {
                ++contiguous_bits;

                if (first_bit == -1)
                    first_bit = i;
            } else {
                contiguous_bits = 0;
                first_bit = -1;
            }

            if (contiguous_bits == length)
                return first_bit;
        }

        return {};
    }

    void set_size(size_t bit_count)
    {
        m_bit_count = bit_count;

        auto units_needed = ceiling_divide<size_t>(m_bit_count, bits_per_unit);

        if (m_bits.size() < units_needed)
            m_bits.expand_to(units_needed);
    }

    void set_bit(size_t index, bool to)
    {
        ASSERT(index < m_bit_count);

        auto location = location_of_bit(index);

        auto& unit = m_bits[location.first];

        if (to)
            unit |= bit_as_mask(location.second);
        else
            unit &= ~bit_as_mask(location.second);
    }

    void set_range_to_number(size_t at_index, size_t count, size_t number)
    {
        ASSERT(at_index + count <= m_bit_count);
        ASSERT(count == 64 || number <= ((static_cast<storage_unit_type>(1) << count) - 1));

        for (size_t i = 0; i < count; ++i)
            set_bit(at_index + i, ((static_cast<storage_unit_type>(1) << i) & number) >> i);
    }

    void set_range_to(size_t at_index, size_t count, bool value)
    {
        ASSERT(at_index + count <= m_bit_count);

        for (size_t i = 0; i < count; ++i)
            set_bit(at_index + i, value);
    }

    [[nodiscard]] bool bit_at(size_t index) const
    {
        auto location = location_of_bit(index);

        auto& unit = m_bits[location.first];

        return unit & bit_as_mask(location.second);
    }

    size_t range_as_number(size_t index, size_t length) const
    {
        ASSERT(index + length <= m_bit_count);

        size_t number = 0;

        for (size_t i = 0; i < length; ++i)
            number |= static_cast<storage_unit_type>(bit_at(index + i)) << i;

        return number;
    }

private:
    static Pair<size_t, size_t> location_of_bit(size_t bit)
    {
        return make_pair(bit / bits_per_unit, bit % bits_per_unit);
    }

    static constexpr storage_unit_type bit_as_mask(size_t bit)
    {
        return static_cast<storage_unit_type>(1) << bit;
    }

private:
    size_t m_bit_count { 0 };
    DynamicArray<storage_unit_type> m_bits;
};
}
