#include "Blocker.h"
#include "Thread.h"

namespace kernel {

Blocker::Blocker(Thread& blocked_thread, Type type)
    : m_thread(blocked_thread)
    , m_type(type)
{
}

bool Blocker::block()
{
    return m_thread.block(this);
}

void Blocker::unblock()
{
    m_thread.unblock();
}

DiskIOBlocker::DiskIOBlocker(Thread& blocked_thread)
    : Blocker(blocked_thread, Type::DISK_IO)
{
}

}