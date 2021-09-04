#include "Blocker.h"
#include "Scheduler.h"
#include "Thread.h"

namespace kernel {

Blocker::Blocker(Thread& blocked_thread, Type type)
    : m_thread(blocked_thread)
    , m_type(type)
{
}

Blocker::Result Blocker::block()
{
    Scheduler::the().block(*this);
    return m_result;
}

void Blocker::unblock()
{
    Scheduler::the().unblock(*this);
}

void Blocker::set_result(Result result)
{
    auto expected = Result::UNDEFINED;
    if (m_result.compare_and_exchange(&expected, result))
        m_should_block = false;
}

DiskIOBlocker::DiskIOBlocker(Thread& blocked_thread)
    : Blocker(blocked_thread, Type::DISK_IO)
{
}

MutexBlocker::MutexBlocker(Thread& blocked_thread)
    : Blocker(blocked_thread, Type::MUTEX)
{
}

ProcessLoadBlocker::ProcessLoadBlocker(Thread& blocked_thread)
    : Blocker(blocked_thread, Type::PROCESS_LOAD)
{
}

IOBlocker::IOBlocker(Thread& blocked_thread)
    : Blocker(blocked_thread, Type::IO)
{
}

SleepBlocker::SleepBlocker(Thread& blocked_thread, u64 wake_time)
    : Blocker(blocked_thread, Type::SLEEP)
    , m_wake_time(wake_time)
{
}

bool SleepBlocker::should_be_woken_up() const
{
    return m_wake_time <= Timer::nanoseconds_since_boot();
}

IRQBlocker::IRQBlocker(Thread& blocked_thread)
    : Blocker(blocked_thread, Type::IRQ)
{
}

}
