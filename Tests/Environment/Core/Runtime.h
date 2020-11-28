#pragma once

#define ASSERT(x) if (!(x)) throw FailedAssertion(#x, __FILE__, __LINE__)
#define ASSERT_NEVER_REACHED() throw FailedAssertion("Tried to execute a non-reachable area", __FILE__, __LINE__)

namespace runtime {
    // FIXME: We should really be able to throw FailedAssertion from here
    [[noreturn]] inline void panic(const char* message)
    {
        throw message;
    }
}
