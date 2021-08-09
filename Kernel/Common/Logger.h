#pragma once

#include "Core/IO.h"
#include "Core/Runtime.h"
#include "Core/Serial.h"

#include "Conversions.h"
#include "Lock.h"
#include "String.h"
#include "Traits.h"
#include "Types.h"

namespace kernel {

class LogSink {
public:
    virtual void write(StringView string) = 0;
    virtual ~LogSink() = default;
};

class E9LogSink : public LogSink {
public:
    static bool is_supported() { return IO::in8<0xE9>() == 0xE9; }

    void write(StringView string) override
    {
        for (char c : string)
            IO::out8<0xE9>(c);
    }
};

class SerialSink : public LogSink {
public:
    SerialSink() { Serial::initialize<Serial::Port::COM1>(); }

    void write(StringView string) override { Serial::write<Serial::Port::COM1>(string); }
};

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
            ASSERT(m_read_offset != m_ring.m_current_record);

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
        u32 m_read_offset { 0 };
        bool m_met_ourselves { false };
    };

    Iterator begin() { return { *this, m_earliest_record, m_buffer_full }; }
    Iterator end() { return { *this, m_buffer_full ? m_earliest_record : m_current_record }; }

    void write(StringView message)
    {
        for (char c : message)
        {
            auto* current_record = &m_log_records[m_current_record];

            if (c == '\n') {
                advance_log_record();
                continue;
            }

            if (current_record->length == CharactersPerRow) {
                advance_log_record();
                current_record = &m_log_records[m_current_record];
            }

            if (m_current_record_untouched) {
                if (m_current_record == RowCapacity - 1)
                    m_buffer_full = true;

                m_current_record_untouched = false;
                current_record->length = 0;

                advance_earliest_record_if_needed();
            }

            current_record->message[current_record->length++] = c;
        }
    }

private:
    void advance_log_record()
    {
        if (++m_current_record == RowCapacity)
            m_current_record = 0;

        m_current_record_untouched = true;
    }

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
    static constexpr size_t bytes_per_log_record = CharactersPerRow + 1;

    static_assert(bytes_per_log_record < 128, "Too many characters per row");
    static_assert(CharactersPerRow != 0, "Invalid character count per row");
    static_assert(RowCapacity != 0, "Row capacity cannot be 0");

    struct LogRecord {
        char message[CharactersPerRow];
        u8 length;
    };

    LogRecord m_log_records[RowCapacity] {};
    u32 m_earliest_record { 0 };
    u32 m_current_record { 0 };
    bool m_current_record_untouched { true };
    bool m_buffer_full { false };
};

class MemorySink : public LogSink {
public:
    // should be about 128 * 120, (16 pages) of memory
    using StaticLogRing = LogRing<128, 119>;

    MemorySink() { new (s_lock_buf) InterruptSafeSpinLock(); }

    void write(StringView string) override
    {
        if (string.starts_with("[\33"_sv))
            return;

        LOCK_GUARD(lock());
        s_ring.write(string);
    }

    static InterruptSafeSpinLock& lock()
    {
        return *reinterpret_cast<InterruptSafeSpinLock*>(s_lock_buf);
    }

    static StaticLogRing& log_ring() { return s_ring; }

private:
    inline static StaticLogRing s_ring;
    alignas(InterruptSafeSpinLock) inline static char s_lock_buf[sizeof(InterruptSafeSpinLock)];
};

template <size_t MaxSinks>
class StaticSinkHolder {
public:
    template <typename T>
    void construct()
    {
        static_assert(is_base_of_v<LogSink, T>, "T must be derived from LogSink");

        // Any log sink _must_ only contain the vtable pointer and nothing else
        static_assert(sizeof(T) == sizeof(ptr_t));

        ASSERT(m_sinks != MaxSinks);

        new (end()) T();
        ++m_sinks;
    }

    LogSink* begin() { return reinterpret_cast<LogSink*>(m_storage); }
    LogSink* end() { return begin() + m_sinks; }

private:
    size_t m_sinks { 0 };
    alignas(sizeof(ptr_t)) u8 m_storage[sizeof(ptr_t) * MaxSinks];
};

inline constexpr size_t sink_holder_capacity = 3;
using DefaultSinkHolder = StaticSinkHolder<sink_holder_capacity>;

alignas(DefaultSinkHolder) inline static u8 sink_storage[sizeof(DefaultSinkHolder)];
alignas(InterruptSafeSpinLock) inline static u8 lock_storage[sizeof(InterruptSafeSpinLock)];

class Logger : public StreamingFormatter<Logger> {
public:
    static constexpr StringView info_prefix = "[\33[34mINFO\033[0m] "_sv;
    static constexpr StringView warn_prefix = "[\33[33mWARNING\033[0m] "_sv;
    static constexpr StringView error_prefix = "[\033[91mERROR\033[0m] "_sv;

    static void initialize()
    {
        ASSERT(s_sinks == nullptr);
        ASSERT(s_write_lock == nullptr);

        s_sinks = new (sink_storage) DefaultSinkHolder();
        s_write_lock = new (lock_storage) InterruptSafeSpinLock;

        if (E9LogSink::is_supported())
            s_sinks->construct<E9LogSink>();

        s_sinks->construct<SerialSink>();
        s_sinks->construct<MemorySink>();
    }

    static bool is_initialized() { return s_sinks && s_write_lock; }

    static StaticSinkHolder<3>& sinks()
    {
        ASSERT(s_sinks != nullptr);

        return *s_sinks;
    }

    static InterruptSafeSpinLock& write_lock()
    {
        return *s_write_lock;
    }

    Logger(bool should_lock = true)
        : m_should_unlock(should_lock)
    {
        if (!is_initialized())
            return;

        if (should_lock)
            write_lock().lock(m_interrupt_state);
    }

    Logger(Logger&& other)
        : m_interrupt_state(other.m_interrupt_state)
        , m_should_unlock(other.m_should_unlock)
    {
        other.m_should_terminate = false;
    }

    void write(StringView string)
    {
        if (!is_initialized())
            return;

        for (auto& sink : sinks())
            sink.write(string);
    }

    ~Logger()
    {
        if (!is_initialized())
            return;

        if (m_should_terminate) {
            write("\n");

            if (m_should_unlock)
                write_lock().unlock(m_interrupt_state);
        }
    }

private:
    bool m_should_terminate { true };
    bool m_interrupt_state { false };
    bool m_should_unlock;

    static inline DefaultSinkHolder* s_sinks;
    static inline InterruptSafeSpinLock* s_write_lock;
};

class DummyLogger {
public:
    template <typename T>
    StreamingFormatter<DummyLogger>::enable_if_default_serializable_t<T> operator<<(T) { return *this; }

    template <typename T>
    StreamingFormatter<DummyLogger>::enable_if_default_ref_serializable_t<T> operator<<(const T&) { return *this; }
};

inline Logger info()
{
    Logger logger;

    logger.write(Logger::info_prefix);

    return logger;
}

inline Logger info(StringView prefix)
{
    auto logger = info();

    logger.write(prefix);
    logger.write(": "_sv);

    return logger;
}

inline Logger log()
{
    return info();
}

inline Logger log(StringView prefix)
{
    return info(prefix);
}

inline Logger warning()
{
    Logger logger;

    logger.write(Logger::warn_prefix);

    return logger;
}

inline Logger warning(StringView prefix)
{
    auto logger = warning();

    logger.write(prefix);
    logger.write(": "_sv);

    return logger;
}

inline Logger error()
{
    Logger logger(false);

    logger.write(Logger::error_prefix);

    return logger;
}

inline Logger error(StringView prefix)
{
    auto logger = error();

    logger.write(prefix);
    logger.write(": "_sv);

    return logger;
}

}
