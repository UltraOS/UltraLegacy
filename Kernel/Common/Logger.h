#pragma once

#include "Core/IO.h"
#include "Core/Runtime.h"

#include "Conversions.h"
#include "Lock.h"
#include "String.h"
#include "Traits.h"
#include "Types.h"

namespace kernel {

class Logger;

enum class format {
    as_dec,
    as_hex,
    as_address, // force format char* as address
};

class AutoLogger {
public:
    AutoLogger(Logger& logger, bool should_lock = true);
    AutoLogger(AutoLogger&& logger);

    AutoLogger& operator<<(const char* string);
    AutoLogger& operator<<(StringView string);
    AutoLogger& operator<<(bool value);
    AutoLogger& operator<<(format option);
    AutoLogger& operator<<(Address addr);

    template <typename T>
    enable_if_t<is_arithmetic_v<T>, AutoLogger&> operator<<(T number);

    template <typename T>
    enable_if_t<is_pointer_v<T>, AutoLogger&> operator<<(T pointer);

    ~AutoLogger();

private:
    Logger& m_logger;
    format  m_format { format::as_dec };
    bool    m_should_terminate { true };
    bool    m_interrupt_state { false };
    bool    m_should_unlock;
};

class Logger {
public:
    static constexpr StringView info_prefix  = "[\33[34mINFO\033[0m] "_sv;
    static constexpr StringView warn_prefix  = "[\33[33mWARNING\033[0m] "_sv;
    static constexpr StringView error_prefix = "[\033[91mERROR\033[0m] "_sv;

    virtual Logger& write(StringView text) = 0;

    void lock(bool& interrupt_state) { m_lock.lock(interrupt_state); }
    void unlock(bool interrupt_state) { m_lock.unlock(interrupt_state); }

    virtual ~Logger() = default;

private:
    InterruptSafeSpinLock m_lock;
};

// Uses port 0xE9 to log
class E9Logger final : public Logger {
private:
    constexpr static u8 log_port = 0xE9;

public:
    Logger& write(StringView string) override
    {
        for (char c: string)
            IO::out8<log_port>(c);

        return s_instance;
    }

    static Logger& get() { return s_instance; }

private:
    static E9Logger s_instance;
};

inline E9Logger E9Logger::s_instance;

inline AutoLogger::AutoLogger(Logger& logger, bool should_lock) : m_logger(logger), m_should_unlock(should_lock)
{
    if (should_lock)
        m_logger.lock(m_interrupt_state);
}

inline AutoLogger::AutoLogger(AutoLogger&& other)
    : m_logger(other.m_logger), m_interrupt_state(other.m_interrupt_state), m_should_unlock(other.m_should_unlock)
{
    other.m_should_terminate = false;
}

inline AutoLogger& AutoLogger::operator<<(const char* string)
{
    return this->operator<<(StringView(string));
}

inline AutoLogger& AutoLogger::operator<<(StringView string)
{
    if (m_format == format::as_address)
        return this->operator<<(reinterpret_cast<const void*>(string.data()));

    m_logger.write(string);

    return *this;
}

inline AutoLogger& AutoLogger::operator<<(format option)
{
    m_format = option;

    return *this;
}

inline AutoLogger& AutoLogger::operator<<(bool value)
{
    if (value)
        m_logger.write("true");
    else
        m_logger.write("false");

    return *this;
}

inline AutoLogger& AutoLogger::operator<<(Address addr)
{
    return *this << addr.as_pointer<void>();
}

template <typename T>
enable_if_t<is_arithmetic_v<T>, AutoLogger&> AutoLogger::operator<<(T number)
{
    constexpr size_t max_number_size = 21;

    char number_as_string[max_number_size];

    if (m_format == format::as_hex && to_hex_string(number, number_as_string, max_number_size))
        m_logger.write(number_as_string);
    else if (to_string(number, number_as_string, max_number_size))
        m_logger.write(number_as_string);
    else
        m_logger.write("<INVALID NUMBER>");

    return *this;
}

template <typename T>
enable_if_t<is_pointer_v<T>, AutoLogger&> AutoLogger::operator<<(T pointer)
{
    constexpr size_t max_number_size = 21;

    char number_as_string[max_number_size];

    // pointers should always be hex
    if (to_hex_string(reinterpret_cast<ptr_t>(pointer), number_as_string, max_number_size))
        m_logger.write(number_as_string);
    else
        m_logger.write("<INVALID NUMBER>");

    return *this;
}

inline AutoLogger::~AutoLogger()
{
    if (m_should_terminate) {
        m_logger.write("\n");

        if (m_should_unlock)
            m_logger.unlock(m_interrupt_state);
    }
}

inline AutoLogger info()
{
    AutoLogger logger(E9Logger::get());

    logger << Logger::info_prefix;

    return logger;
}

inline AutoLogger warning()
{
    AutoLogger logger(E9Logger::get());

    logger << Logger::warn_prefix;

    return logger;
}

inline AutoLogger log()
{
    return info();
}

inline Address vga_log(StringView string, size_t row, size_t column, u8 color)
{
    static constexpr size_t vga_columns        = 80;
    static constexpr size_t vga_bytes_per_char = 2;
    static constexpr ptr_t  vga_address        = 0xB8000;
#ifdef ULTRA_32
    static constexpr ptr_t linear_vga_address = 3 * GB + vga_address;
#elif defined(ULTRA_64)
    static constexpr ptr_t linear_vga_address = 0xFFFF800000000000 + vga_address;
#endif

    ptr_t initial_memory = linear_vga_address + vga_columns * vga_bytes_per_char * row;

    u16* memory = reinterpret_cast<u16*>(linear_vga_address + vga_columns * vga_bytes_per_char * row + column);

    for (char c: string) {
        u16 colored_char = c;
        colored_char |= color << 8;

        *(memory++) = colored_char;
    }

    return reinterpret_cast<ptr_t>(memory) - initial_memory;
};

inline AutoLogger error()
{
    AutoLogger logger(E9Logger::get(), false);

    logger << Logger::error_prefix;

    return logger;
}

}
