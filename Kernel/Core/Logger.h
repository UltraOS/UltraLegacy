#pragma once

#include "Types.h"
#include "IO.h"

namespace kernel {
    class Logger;

    class AutoLogger
    {
    public:
        AutoLogger(Logger& logger);
        AutoLogger(AutoLogger&& logger);

        AutoLogger& operator<<(const char* string);

        ~AutoLogger();
    private:
        Logger& m_logger;
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

    inline AutoLogger::~AutoLogger()
    {
        m_logger.write("\n");
    }

    inline AutoLogger info()
    {
        auto& logger = E9Logger::get();

        logger.write("\33[34m[INFO]\033[0m ");

        return AutoLogger(logger);
    }

    inline AutoLogger warning()
    {
        auto& logger = E9Logger::get();

        logger.write("\33[33m[WARNING]\033[0m ");

        return AutoLogger(logger);
    }

    inline AutoLogger error()
    {
        auto& logger = E9Logger::get();

        logger.write("\033[91m[ERROR]\033[0m ");

        return AutoLogger(logger);
    }

    inline AutoLogger log()
    {
        return info();
    }
}
