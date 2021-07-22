#include "Common/Logger.h"

#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"
#include "Multitasking/Sleep.h"

#include "FileSystem/IOStream.h"
#include "FileSystem/VFS.h"

#include "WindowManager/WindowManager.h"

#include "Syscall.h"

namespace kernel {

void(*Syscall::s_table[static_cast<size_t>(Syscall::NumberOf::MAX) + 1])(RegisterState&) =
{
#define SYSCALL(name) &Syscall::name,
ENUMERATE_SYSCALLS
#undef SYSCALL
};

#ifdef ULTRA_32
#define RETVAL registers.eax
#define FUNCTION registers.eax
#define ARG0 registers.ebx
#define ARG1 registers.ecx
#define ARG2 registers.edx
#elif defined(ULTRA_64)
#define RETVAL registers.rax
#define FUNCTION registers.rax
#define ARG0 registers.rbx
#define ARG1 registers.rcx
#define ARG2 registers.rdx
#endif

void Syscall::invoke(RegisterState& registers)
{
    if (FUNCTION >= static_cast<size_t>(NumberOf::MAX)) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    s_table[FUNCTION](registers);
}

SYSCALL_IMPLEMENTATION(EXIT)
{
    Thread::current()->set_invulnerable(false);
    Scheduler::the().exit(ARG0);
}

SYSCALL_IMPLEMENTATION(OPEN)
{
    auto path = String(Address(ARG0).as_pointer<const char>());

    if (!path.starts_with("/")) {
        auto& current_process = Process::current();

        LOCK_GUARD(current_process.lock());
        String full_path = current_process.working_directory();
        full_path += path;
        path = move(full_path);
    }

    auto file = VFS::the().open(path.to_view(),
                              static_cast<IOMode>(ARG1));

    if (file.is_error()) {
        log() << "error " << file.error().to_string();
        RETVAL = -(i64)file.error().value;
        return;
    }

    auto id = Process::current().store_io_stream(file.value());
    if (id.is_error()) {
        file.value()->close();
        RETVAL = -id.error().value;
        return;
    }

    RETVAL = id.value();
}

SYSCALL_IMPLEMENTATION(CLOSE)
{
    auto stream = Process::current().pop_io_stream(ARG0);
    if (!stream) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    RETVAL = -stream->close().value;
}

SYSCALL_IMPLEMENTATION(READ)
{
    auto stream = Process::current().io_stream(ARG0);

    if (!stream) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    for (;;) {
        if (!stream->can_read_without_blocking()) {
            auto ret = stream->block_until_readable();
            if (ret.is_error()) {
                RETVAL = -ret.error().value;
                return;
            }

            if (ret.value() == Blocker::Result::INTERRUPTED) {
                RETVAL = -ErrorCode::INTERRUPTED;
                return;
            }
        }

        auto ret = stream->read(Address(ARG1).as_pointer<void>(), ARG2);

        if (ret.is_error()) {
            RETVAL = -ret.error().value;
            return;
        }

        if (ret.value() == 0)
            continue;

        RETVAL = ret.value();
        return;
    }
}

SYSCALL_IMPLEMENTATION(WRITE)
{
    auto stream = Process::current().io_stream(ARG0);

    if (!stream) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    for (;;) {
        if (!stream->can_write_without_blocking()) {
            auto ret = stream->block_until_writable();
            if (ret.is_error()) {
                RETVAL = -ret.error().value;
                return;
            }

            if (ret.value() == Blocker::Result::INTERRUPTED) {
                RETVAL = -ErrorCode::INTERRUPTED;
                return;
            }
        }

        auto ret = stream->write(Address(ARG1).as_pointer<void>(), ARG2);

        if (ret.is_error()) {
            RETVAL = -ret.error().value;
            return;
        }

        if (ret.value() == 0)
            continue;

        RETVAL = ret.value();
        return;
    }
}

SYSCALL_IMPLEMENTATION(SEEK)
{
    auto stream = Process::current().io_stream(ARG0);

    if (!stream) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    auto error_or_offset = stream->seek(ARG1, static_cast<SeekMode>(ARG2));

    if (error_or_offset.is_error())
        RETVAL = -error_or_offset.error().value;
    else
        RETVAL = error_or_offset.value();
}

SYSCALL_IMPLEMENTATION(TRUNCATE)
{
    auto file = Process::current().io_stream(ARG0);

    if (!file || file->type() != IOStream::Type::FILE_ITERATOR) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    RETVAL = -static_cast<FileIterator*>(file.get())->truncate(ARG1).value;
}

SYSCALL_IMPLEMENTATION(CREATE)
{
    RETVAL = -VFS::the().create(reinterpret_cast<const char*>(ARG0)).value;
}

SYSCALL_IMPLEMENTATION(CREATE_DIR)
{
    RETVAL = -VFS::the().create_directory(reinterpret_cast<const char*>(ARG0)).value;
}

SYSCALL_IMPLEMENTATION(REMOVE)
{
    RETVAL = -VFS::the().remove(reinterpret_cast<const char*>(ARG0)).value;
}

SYSCALL_IMPLEMENTATION(REMOVE_DIR)
{
    RETVAL = -VFS::the().remove_directory(reinterpret_cast<const char*>(ARG0)).value;
}

SYSCALL_IMPLEMENTATION(MOVE)
{
    RETVAL = -VFS::the().move(reinterpret_cast<const char*>(ARG0), reinterpret_cast<const char*>(ARG1)).value;
}

SYSCALL_IMPLEMENTATION(VIRTUAL_ALLOC)
{
    auto range = Range(ARG0, Page::round_up(ARG1));

    if (!Page::is_aligned(range.begin())) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    MemoryManager::VR vr;

    if (range.begin()) {
        bool is_invalid = range.begin() < MemoryManager::userspace_usable_base || range.end() > MemoryManager::userspace_usable_ceiling;

        if (is_invalid) {
            RETVAL = -ErrorCode::INVALID_ARGUMENT;
            return;
        }

        vr = MemoryManager::the().allocate_user_private("Generic Private"_sv, range);
    } else {
        vr = MemoryManager::the().allocate_user_private_anywhere("Generic Private"_sv, range.length());
    }

    auto& process = Process::current();

    {
        LOCK_GUARD(process.lock());
        process.virtual_regions().emplace(vr);
    }

    RETVAL = vr->virtual_range().begin();
}

SYSCALL_IMPLEMENTATION(VIRTUAL_FREE)
{
    auto range = Range(ARG0, ARG1);

    auto& process = Process::current();
    auto& vrs = process.virtual_regions();

    LOCK_GUARD(process.lock());

    // TODO: allow freeing in the middle of the region
    auto region = vrs.lower_bound(range.begin());
    if (region == vrs.end() || (*region)->virtual_range().begin() != range.begin() || (*region)->virtual_range().length() != range.length()) {
        RETVAL = -ErrorCode::INVALID_ARGUMENT;
        return;
    }

    MemoryManager::the().free_virtual_region(**region);
    vrs.remove(region);

    RETVAL = ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(WM_COMMAND)
{
    RETVAL = -WindowManager::the().dispatch_window_command(reinterpret_cast<void*>(ARG0)).value;
}

SYSCALL_IMPLEMENTATION(SLEEP)
{
    Thread::current()->set_invulnerable(false);
    sleep::for_nanoseconds(ARG0);

    RETVAL = ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(TICKS)
{
    RETVAL = Timer::nanoseconds_since_boot() / Time::nanoseconds_in_millisecond;
}

SYSCALL_IMPLEMENTATION(DEBUG_LOG)
{
    auto string = StringView(Address(ARG0).as_pointer<const char>());
    log() << "P[" << Process::current().id() << "][" << Thread::current()->id() << "] log: " << string;

    RETVAL = ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(MAX)
{
    runtime::panic("Invoked MAX syscall");
}

SYSCALL_IMPLEMENTATION(CREATE_PROCESS)
{
    // TODO
    RETVAL = -ErrorCode::UNSUPPORTED;
}

SYSCALL_IMPLEMENTATION(CREATE_THREAD)
{
    RETVAL = -ErrorCode::UNSUPPORTED;
}

}
