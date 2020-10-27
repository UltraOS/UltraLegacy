#pragma once

#include "Utilities.h"

#include "Common/Types.h"

#include "ExceptionDispatcher.h"

namespace kernel {

class ExceptionHandler {
    MAKE_NONCOPYABLE(ExceptionHandler)
public:
    ExceptionHandler(size_t exception_number)
        : m_exception_number(exception_number)
    {
        ExceptionDispatcher::register_handler(*this);
    }

    size_t exception_number() const { return m_exception_number; }

    virtual void handle(const RegisterState& registers) = 0;

    virtual ~ExceptionHandler() { ExceptionDispatcher::unregister_handler(*this); }

private:
    size_t m_exception_number;
};
}
