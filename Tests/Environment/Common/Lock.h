#pragma once

#include <iostream>
#include <cstring>


namespace kernel {

class InterruptSafeSpinLock
{
public:
    void lock(bool) {}
    void unlock(bool) {}
};
}
