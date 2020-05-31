#pragma once

#include "Types.h"
#include "IO.h"
#include "Traits.h"
#include "Conversions.h"

namespace kernel {
    class Logger;

    enum class format
    {
        as_dec,
        as_hex,
    };

    class AutoLogger
    {
    public:
        AutoLogger(Logger& logger);
        AutoLogger(AutoLogger&& logger);

        AutoLogger& operator<<(const char* string);
        AutoLogger& operator<<(bool value);
        AutoLogger& operator<<(format option);

        template<typename T>
        enable_if_t<is_integral<T>::value, AutoLogger&> operator<<(T number);

        ~AutoLogger();
    private:
        Logger& m_logger;
        format m_format { format::as_dec };
        bool should_terminate;
    };

    class Logger
    {
    public:
        virtual Logger& write(const char* text) = 0;

        // TODO: add [virtual ~Logger() = default;] once I have operator delete
    };

    // Uses port 0xE9 to log
    class E9Logger final : public Logger
    {
    private:
        constexpr static u8 log_port = 0xE9;
        static E9Logger s_instance;
    public:
        Logger& write(const char* text) override
        {
            while (*text)
                IO::out8<log_port>(*(text++));

            return s_instance;
        }

        static Logger& get()
        {
            return s_instance;
        }
    };

    inline E9Logger E9Logger::s_instance;

    inline AutoLogger::AutoLogger(Logger& logger)
        : m_logger(logger), should_terminate(true)
    {
    }

    inline AutoLogger::AutoLogger(AutoLogger&& other)
        : m_logger(other.m_logger), should_terminate(true)
    {
        other.should_terminate = false;
    }

    inline AutoLogger& AutoLogger::operator<<(const char* string)
    {
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

    template<typename T>
    enable_if_t<is_integral<T>::value, AutoLogger&> AutoLogger::operator<<(T number)
    {
        constexpr size_t max_number_size = 21;

        char number_as_string[max_number_size];
        bool ok;

        if (m_format == format::as_hex)
            to_hex_string(number, number_as_string, max_number_size, ok);
        else
            to_string(number, number_as_string, max_number_size, ok);

        if (ok)
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
}
