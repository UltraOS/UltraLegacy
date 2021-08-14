#pragma once

#include "Types.h"
#include "String.h"

namespace kernel {

template <size_t RowCapacity, size_t CharactersPerRow>
class LogRing {
public:
    static constexpr size_t row_capacity() { return RowCapacity; }
    static constexpr size_t characters_per_pow() { return CharactersPerRow; }

    class Iterator
    {
    public:
        Iterator(LogRing& ring, size_t read_offset, bool should_meet_ourselves = false)
            : m_ring(ring)
            , m_read_offset(read_offset)
            , m_met_ourselves(!should_meet_ourselves)
        {
        }

        StringView operator*()
        {
            auto& log = m_ring.m_log_records[m_read_offset];
            return { log.message, log.length };
        }

        Iterator& operator++()
        {
            if (++m_read_offset == m_ring.row_capacity())
                m_read_offset = 0;

            return *this;
        }

        bool operator!=(const Iterator& itr)
        {
            if (m_read_offset == itr.m_read_offset) {
                if (m_met_ourselves)
                    return false;

                m_met_ourselves = true;
                return true;
            }

            return m_read_offset != itr.m_read_offset;
        }

    private:
        LogRing& m_ring;
        u32 m_read_offset{ 0 };
        bool m_met_ourselves{ false };
    };

    Iterator begin()
    {
        bool should_wrap = false;

        // If buffer is full, earliest record is always the end of log as well. Range is [first_record, first_record)
        should_wrap |= m_buffer_full;

        // Edge case: buffer is not full but last record is currently active, so end of log wraps around to the above case.
        should_wrap |= !m_current_record_untouched && (m_current_record + 1 == RowCapacity);

        return { *this, m_earliest_record, should_wrap };
    }

    Iterator end()
    {
        if (m_buffer_full)
            return { *this, m_earliest_record };

        auto end_record = m_current_record;

        // If we wrote at least 1 byte to the current record, it's now included in the iteration range.
        end_record += !m_current_record_untouched;

        // Edge case: buffer is not full but last record is currently active, so end of log wraps around.
        if (end_record == RowCapacity)
            end_record = 0;

        return { *this, end_record };
    }

    void write(StringView message)
    {
        for (char c : message)
        {
            auto* current_record = &m_log_records[m_current_record];

            if ((!m_current_record_untouched && current_record->length == CharactersPerRow) || c == '\n') {
                // skip useless newlines
                if (m_current_record_untouched)
                    continue;

                if (++m_current_record == RowCapacity) {
                    m_current_record = 0;
                    m_buffer_full = true;
                }

                current_record = &m_log_records[m_current_record];
                m_current_record_untouched = true;

                if (c == '\n')
                    continue;
            }

            if (m_current_record_untouched) {
                advance_earliest_record_if_needed();
                m_current_record_untouched = false;
                current_record->length = 0;
            }

            current_record->message[current_record->length++] = c;
        }
    }

private:
    void advance_earliest_record_if_needed()
    {
        if (!m_buffer_full)
            return;

        if (m_current_record == m_earliest_record)
            ++m_earliest_record;

        if (m_earliest_record == RowCapacity)
            m_earliest_record = 0;
    }

private:
    static_assert(CharactersPerRow < 128, "Too many characters per row");
    static_assert(CharactersPerRow != 0, "Invalid character count per row");
    static_assert(RowCapacity != 0, "Row capacity cannot be 0");

    struct LogRecord {
        char message[CharactersPerRow];
        u8 length;
    };

    LogRecord m_log_records[RowCapacity]{};
    u32 m_earliest_record { 0 };
    u32 m_current_record { 0 };
    bool m_current_record_untouched { true };
    bool m_buffer_full { false };
};

}
