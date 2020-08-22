#pragma once

#include "Core/DebugTerminal.h"
#include "Core/IO.h"
#include "Core/Runtime.h"

#include "Conversions.h"
#include "Lock.h"
#include "String.h"
#include "Traits.h"
#include "Types.h"

namespace kernel {

class LogSink {
public:
    virtual void write(StringView string) = 0;
    virtual ~LogSink()                    = default;
};

class E9LogSink : public LogSink {
public:
    static bool is_supported() { return IO::in8<0xE9>() == 0xE9; }

    void write(StringView string) override
    {
        for (char c: string)
            IO::out8<0xE9>(c);
    }
};

class TerminalSink : public LogSink {
public:
    void write(StringView string) override
    {
        if (DebugTerminal::is_initialized())
            DebugTerminal::the().write(string);
    }
};

enum class format {
    as_dec,
    as_hex,
    as_address, // force format char* as address
};

class Logger {
public:
    static constexpr StringView info_prefix  = "[\33[34mINFO\033[0m] "_sv;
    static constexpr StringView warn_prefix  = "[\33[33mWARNING\033[0m] "_sv;
    static constexpr StringView error_prefix = "[\033[91mERROR\033[0m] "_sv;

    static void initialize()
    {
        ASSERT(s_sinks == nullptr);
        ASSERT(s_write_lock == nullptr);

        s_sinks = new DynamicArray<LogSink*>(2);

        if (E9LogSink::is_supported())
            s_sinks->emplace(new E9LogSink());

        s_sinks->emplace(new TerminalSink());

        s_write_lock = new InterruptSafeSpinLock;
    }

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

    void write(StringView string)
    {
        for (auto& sink: sinks())
            sink->write(string);
    }

    Logger(bool should_lock = true) : m_should_unlock(should_lock)
    {
        if (should_lock)
            write_lock().lock(m_interrupt_state);
    }

    Logger(Logger&& other) : m_interrupt_state(other.m_interrupt_state), m_should_unlock(other.m_should_unlock)
    {
        other.m_should_terminate = false;
    }

    Logger& operator<<(const char* string) { return this->operator<<(StringView(string)); }

    Logger& operator<<(StringView string)
    {
        if (m_format == format::as_address)
            return this->operator<<(reinterpret_cast<const void*>(string.data()));

        write(string);

        return *this;
    }

    Logger& operator<<(format option)
    {
        m_format = option;

        return *this;
    }

    Logger& operator<<(bool value)
    {
        if (value)
            write("true");
        else
            write("false");

        return *this;
    }

    inline Logger& operator<<(Address addr) { return *this << addr.as_pointer<void>(); }

    template <typename T>
    enable_if_t<is_arithmetic_v<T>, Logger&> operator<<(T number)
    {
        constexpr size_t max_number_size = 21;

        char number_as_string[max_number_size];

        if (m_format == format::as_hex && to_hex_string(number, number_as_string, max_number_size))
            write(number_as_string);
        else if (to_string(number, number_as_string, max_number_size))
            write(number_as_string);
        else
            write("<INVALID NUMBER>");

        return *this;
    }

    template <typename T>
    enable_if_t<is_pointer_v<T>, Logger&> operator<<(T pointer)
    {
        constexpr size_t max_number_size = 21;

        char number_as_string[max_number_size];

        // pointers should always be hex
        if (to_hex_string(reinterpret_cast<ptr_t>(pointer), number_as_string, max_number_size))
            write(number_as_string);
        else
            write("<INVALID NUMBER>");

        return *this;
    }

    ~Logger()
    {
        if (m_should_terminate) {
            write("\n");

            if (m_should_unlock)
                write_lock().unlock(m_interrupt_state);
        }
    }

private:
    format m_format { format::as_dec };
    bool   m_should_terminate { true };
    bool   m_interrupt_state { false };
    bool   m_should_unlock;

    static inline DynamicArray<LogSink*>* s_sinks;
    static inline InterruptSafeSpinLock*  s_write_lock;
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
