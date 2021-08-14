#pragma once

#include "Core/IO.h"
#include "Core/Runtime.h"
#include "Core/Serial.h"

#include "Conversions.h"
#include "Lock.h"
#include "String.h"
#include "LogRing.h"
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

class MemorySink : public LogSink {
public:
    // should be about 128 * 120, (16 pages) of memory
    using StaticLogRing = LogRing<128, 119>;

    MemorySink() { new (s_lock_buf) InterruptSafeSpinLock(); }

    void write(StringView string) override
    {
        if (string.starts_with("[\33"_sv))
            return;

        auto& wlock = lock();
        if (wlock.is_deadlocked())
            return;

        LOCK_GUARD(wlock);
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
