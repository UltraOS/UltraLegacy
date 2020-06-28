#pragma once

#include "Conversions.h"
#include "Core/IO.h"
#include "Core/Runtime.h"
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
    AutoLogger(Logger& logger);
    AutoLogger(AutoLogger&& logger);

    AutoLogger& operator<<(const char* string);
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
    bool    should_terminate;
};

class Logger {
public:
    virtual Logger& write(const char* text) = 0;

    virtual ~Logger() = default;
};

// Uses port 0xE9 to log
class E9Logger final : public Logger {
private:
    constexpr static u8 log_port = 0xE9;

public:
    Logger& write(const char* text) override
    {
        while (*text)
            IO::out8<log_port>(*(text++));

        return s_instance;
    }

    static Logger& get() { return s_instance; }

private:
    static E9Logger s_instance;
};

inline E9Logger E9Logger::s_instance;

inline AutoLogger::AutoLogger(Logger& logger) : m_logger(logger), should_terminate(true) { }

inline AutoLogger::AutoLogger(AutoLogger&& other) : m_logger(other.m_logger), should_terminate(true)
{
    other.should_terminate = false;
}

inline AutoLogger& AutoLogger::operator<<(const char* string)
{
    if (m_format == format::as_address)
        return this->operator<<(reinterpret_cast<const void*>(string));

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
    m_logger.write("\n");
}

inline AutoLogger info()
{
    auto& logger = E9Logger::get();

    logger.write("[\33[34mINFO\033[0m] ");

    return AutoLogger(logger);
}

inline AutoLogger warning()
{
    auto& logger = E9Logger::get();

    logger.write("[\33[33mWARNING\033[0m] ");

    return AutoLogger(logger);
}

inline AutoLogger error()
{
    auto& logger = E9Logger::get();

    logger.write("[\033[91mERROR\033[0m] ");

    return AutoLogger(logger);
}

inline AutoLogger log()
{
    return info();
}

inline Address vga_log(const char* string, size_t row, size_t column, u8 color)
{
    static constexpr size_t vga_columns        = 80;
    static constexpr size_t vga_bytes_per_char = 2;
    static constexpr ptr_t  vga_address        = 0xB8000;
    static constexpr ptr_t  linear_vga_address = 3 * GB + vga_address;

    ptr_t initial_memory = linear_vga_address + vga_columns * vga_bytes_per_char * row;

    u16* memory = reinterpret_cast<u16*>(linear_vga_address + vga_columns * vga_bytes_per_char * row + column);

    while (*string) {
        u16 colored_char = *(string++);
        colored_char |= color << 8;

        *(memory++) = colored_char;
    }

    return reinterpret_cast<ptr_t>(memory) - initial_memory;
};
}
