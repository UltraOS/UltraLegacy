#pragma once

#include "Types.h"
#include "IO.h"

namespace kernel {
    class Logger
    {
    public:
        virtual Logger& operator<<(const char* text) = 0;
        virtual Logger& write(const char* text) = 0;

        // TODO: add [virtual ~Logger() = default;] once I have operator delete
    };

    // Uses port 0xE9 to log
    class E9Logger final : public Logger
    {
    private:
        static E9Logger s_instance;
    public:
        Logger& operator<<(const char* text) override
        {
            return write(text);
        }

        Logger& write(const char* text) override
        {
            while (*text)
                IO::out8<0xE9>(*(text++));

            return s_instance;
        }

        static Logger& get()
        {
            return s_instance;
        }
    };

    inline Logger& log()
    {
        return E9Logger::get();
    }

    inline E9Logger E9Logger::s_instance;
}