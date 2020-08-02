#pragma once

#include "Atomic.h"

namespace kernel {

class SpinLock {
public:
    using lock_t = size_t;

private:
    Atomic<size_t> m_lock;
};
}
