#pragma once

namespace kernel {

class InterruptSafeSpinLock
{
public:
    void lock(bool) {}
    void unlock(bool) {}
};
}
