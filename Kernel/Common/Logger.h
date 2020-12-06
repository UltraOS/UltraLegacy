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

class Logger : public StreamingFormatter {
public:
    static constexpr StringView info_prefix = "[\33[34mINFO\033[0m] "_sv;
    static constexpr StringView warn_prefix = "[\33[33mWARNING\033[0m] "_sv;
    static constexpr StringView error_prefix = "[\033[91mERROR\033[0m] "_sv;

    static void initialize()
    {
        ASSERT(s_sinks == nullptr);
        ASSERT(s_write_lock == nullptr);

        s_sinks = new DynamicArray<LogSink*>(2);

        if (E9LogSink::is_supported())
            s_sinks->emplace(new E9LogSink());

        s_sinks->emplace(new SerialSink());

        s_write_lock = new InterruptSafeSpinLock;
    }

    static bool is_initialized() { return s_sinks && s_write_lock; }

    static DynamicArray<LogSink*>& sinks()
    {
        ASSERT(s_sinks != nullptr);

        return *s_sinks;
    }

    static InterruptSafeSpinLock& write_lock()
    {
        ASSERT(s_write_lock != nullptr);

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

    template <typename T>
    enable_if_default_serializable_t<T, Logger> operator<<(T value)
    {
        if constexpr (is_same_v<T, format>) {
            set_format(value);
        } else {
            append(value);
        }

        return *this;
    }

    void write(StringView string) override
    {
        if (!is_initialized())
            return;

        for (auto& sink : sinks())
            sink->write(string);
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

    static inline DynamicArray<LogSink*>* s_sinks;
    static inline InterruptSafeSpinLock* s_write_lock;
};

inline Logger info()
{
    Logger logger;

    logger.write(Logger::info_prefix);

    return logger;
}

inline Logger warning()
{
    Logger logger;

    logger.write(Logger::warn_prefix);

    return logger;
}

inline Logger error()
{
    Logger logger(false);

    logger.write(Logger::error_prefix);

    return logger;
}

inline Logger log()
{
    return info();
}
}
