#pragma once

#define ASSERT(x) if (!(x)) throw FailedAssertion(#x, __FILE__, __LINE__)
#define ASSERT_NEVER_REACHED() throw FailedAssertion("Tried to execute a non-reachable area", __FILE__, __LINE__)
