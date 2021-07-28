#include "Common/Logger.h"
#include "Common/Span.h"

#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"
#include "Multitasking/Sleep.h"

#include "FileSystem/IOStream.h"
#include "FileSystem/VFS.h"

#include "Memory/SafeOperations.h"

#include "WindowManager/WindowManager.h"

#include "Syscall.h"

namespace kernel {

ErrorOr<ptr_t>(*Syscall::s_table[static_cast<size_t>(Syscall::NumberOf::MAX) + 1])(RegisterState&) =
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

    auto res = s_table[FUNCTION](registers);

    if (res.is_error())
        RETVAL = -res.error().value;
    else
        RETVAL = res.value();
}

SYSCALL_IMPLEMENTATION(EXIT)
{
    Thread::current()->set_invulnerable(false);
    Scheduler::the().exit(ARG0);
}

static ErrorOr<StringView> copy_user_path(void* user_pointer, Span<char> kernel_buffer)
{
    if (!MemoryManager::is_potentially_valid_userspace_pointer(user_pointer))
        return ErrorCode::MEMORY_ACCESS_VIOLATION;

    size_t bytes = copy_until_null_or_n_from_user(user_pointer, kernel_buffer.data(), kernel_buffer.size());

    if (bytes == 0)
        return ErrorCode::MEMORY_ACCESS_VIOLATION;
    if (bytes == 1 || (bytes == kernel_buffer.size() && kernel_buffer.back() != '\0'))
        return ErrorCode::BAD_PATH;

    // A bunch of workarounds to avoid the kernel heap
    if (kernel_buffer.front() == '/')
        return StringView(kernel_buffer.data(), bytes - 1);

    size_t bytes_left_in_path = kernel_buffer.size() - bytes;
    move_memory(kernel_buffer.data(), kernel_buffer.data() + bytes_left_in_path, bytes);
    char* new_path_begin = nullptr;
    size_t combined_size = bytes - 1;

    auto& current_process = Process::current();
    LOCK_GUARD(current_process.lock());
    auto& work_dir = current_process.working_directory();

    // cannot combine working directory and path
    if (work_dir.size() > bytes_left_in_path)
        return ErrorCode::BAD_PATH;

    combined_size += work_dir.size();
    new_path_begin = kernel_buffer.data() + (bytes_left_in_path - work_dir.size());
    copy_memory(work_dir.data(), new_path_begin, work_dir.size());

    return StringView(new_path_begin, combined_size);
}

SYSCALL_IMPLEMENTATION(OPEN)
{
    char buffer[File::max_name_length + 1];
    auto path = copy_user_path(Address(ARG0).as_pointer<void>(), buffer);

    if (path.is_error())
        return path.error();

    auto file = VFS::the().open(path.value(), static_cast<IOMode>(ARG1));

    if (file.is_error())
        return file.error();

    auto id = Process::current().store_io_stream(file.value());
    if (id.is_error()) {
        file.value()->close();
        return id.error();
    }

    return id.value();
}

SYSCALL_IMPLEMENTATION(CLOSE)
{
    auto stream = Process::current().pop_io_stream(ARG0);
    if (!stream)
        return ErrorCode::INVALID_ARGUMENT;

    return stream->close();
}

SYSCALL_IMPLEMENTATION(READ)
{
    auto stream = Process::current().io_stream(ARG0);

    if (!stream)
        return ErrorCode::INVALID_ARGUMENT;

    if (!MemoryManager::is_potentially_valid_userspace_pointer(ARG1))
        return ErrorCode::MEMORY_ACCESS_VIOLATION;

    for (;;) {
        if (!stream->can_read_without_blocking()) {
            auto ret = stream->block_until_readable();
            if (ret.is_error())
                return ret.error();

            if (ret.value() == Blocker::Result::INTERRUPTED)
                return ErrorCode::INTERRUPTED;
        }

        auto ret = stream->read(Address(ARG1).as_pointer<void>(), ARG2);

        if (ret.is_error())
            return ret.error();

        if (ret.value() == 0)
            continue;

        return ret.value();
    }
}

SYSCALL_IMPLEMENTATION(WRITE)
{
    auto stream = Process::current().io_stream(ARG0);

    if (!stream)
        return ErrorCode::INVALID_ARGUMENT;
    if (!MemoryManager::is_potentially_valid_userspace_pointer(ARG1))
        return ErrorCode::MEMORY_ACCESS_VIOLATION;

    for (;;) {
        if (!stream->can_write_without_blocking()) {
            auto ret = stream->block_until_writable();
            if (ret.is_error())
                return ret.error();

            if (ret.value() == Blocker::Result::INTERRUPTED)
                return ErrorCode::INTERRUPTED;
        }

        auto ret = stream->write(Address(ARG1).as_pointer<void>(), ARG2);

        if (ret.is_error())
            return ret.error();

        if (ret.value() == 0)
            continue;

        return ret.value();
    }
}

SYSCALL_IMPLEMENTATION(SEEK)
{
    auto stream = Process::current().io_stream(ARG0);

    if (!stream)
        return ErrorCode::INVALID_ARGUMENT;

    auto error_or_offset = stream->seek(ARG1, static_cast<SeekMode>(ARG2));

    if (error_or_offset.is_error())
        return error_or_offset.error();

    return error_or_offset.value();
}

SYSCALL_IMPLEMENTATION(TRUNCATE)
{
    auto file = Process::current().io_stream(ARG0);

    if (!file || file->type() != IOStream::Type::FILE_ITERATOR)
        return ErrorCode::INVALID_ARGUMENT;

    return static_cast<FileIterator*>(file.get())->truncate(ARG1);
}

SYSCALL_IMPLEMENTATION(CREATE)
{
    char buffer[File::max_name_length + 1];
    auto path = copy_user_path(Address(ARG0).as_pointer<void>(), buffer);

    if (path.is_error())
        return path.error();

    return VFS::the().create(path.value());
}

SYSCALL_IMPLEMENTATION(CREATE_DIR)
{
    char buffer[File::max_name_length + 1];
    auto path = copy_user_path(Address(ARG0).as_pointer<void>(), buffer);

    if (path.is_error())
        return path.error();

    return VFS::the().create_directory(path.value());
}

SYSCALL_IMPLEMENTATION(REMOVE)
{
    char buffer[File::max_name_length + 1];
    auto path = copy_user_path(Address(ARG0).as_pointer<void>(), buffer);

    if (path.is_error())
        return path.error();

    return VFS::the().remove(path.value());
}

SYSCALL_IMPLEMENTATION(REMOVE_DIR)
{
    char buffer[File::max_name_length + 1];
    auto path = copy_user_path(Address(ARG0).as_pointer<void>(), buffer);

    if (path.is_error())
        return path.error();

    return VFS::the().remove_directory(path.value());
}

SYSCALL_IMPLEMENTATION(MOVE)
{
    char from_buffer[File::max_name_length + 1];
    auto from_path = copy_user_path(Address(ARG0).as_pointer<void>(), from_buffer);

    if (from_path.is_error())
        return from_path.error();

    char to_buffer[File::max_name_length + 1];
    auto to_path = copy_user_path(Address(ARG1).as_pointer<void>(), to_buffer);

    if (to_path.is_error())
        return to_path.error();

    return VFS::the().move(from_path.value(), to_path.value());
}

SYSCALL_IMPLEMENTATION(VIRTUAL_ALLOC)
{
    auto range = Range(ARG0, Page::round_up(ARG1));

    if (!Page::is_aligned(range.begin()))
        return ErrorCode::INVALID_ARGUMENT;

    MemoryManager::VR vr;

    if (range.begin()) {
        if (!MemoryManager::is_potentially_valid_userspace_pointer(range.begin()))
            return ErrorCode::INVALID_ARGUMENT;
        if (!MemoryManager::is_potentially_valid_userspace_pointer(range.end()))
            return ErrorCode::INVALID_ARGUMENT;

        vr = MemoryManager::the().allocate_user_private("Generic Private"_sv, range);
    } else {
        vr = MemoryManager::the().allocate_user_private_anywhere("Generic Private"_sv, range.length());
    }

    auto& process = Process::current();

    {
        LOCK_GUARD(process.lock());
        process.virtual_regions().emplace(vr);
    }

    return vr->virtual_range().begin().raw();
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
        return ErrorCode::INVALID_ARGUMENT;
    }

    MemoryManager::the().free_virtual_region(**region);
    vrs.remove(region);

    return ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(WM_COMMAND)
{
    return WindowManager::the().dispatch_window_command(reinterpret_cast<void*>(ARG0));
}

SYSCALL_IMPLEMENTATION(SLEEP)
{
    Thread::current()->set_invulnerable(false);
    sleep::for_nanoseconds(ARG0);

    return ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(TICKS)
{
    return Timer::nanoseconds_since_boot() / Time::nanoseconds_in_millisecond;
}

SYSCALL_IMPLEMENTATION(DEBUG_LOG)
{
    char log_buffer[512];
    size_t bytes = copy_until_null_or_n_from_user(Address(ARG0).as_pointer<void>(), log_buffer, sizeof(log_buffer));
    if (bytes == 0)
        return ErrorCode::MEMORY_ACCESS_VIOLATION;

    auto message = StringView(log_buffer, bytes - 1);

    log() << "PID[" << Process::current().id() << "]-TID[" << Thread::current()->id() << "] log: " << message;

    return ErrorCode::NO_ERROR;
}

SYSCALL_IMPLEMENTATION(MAX)
{
    runtime::panic("Invoked MAX syscall");
}

SYSCALL_IMPLEMENTATION(CREATE_PROCESS)
{
    // TODO
    return ErrorCode::UNSUPPORTED;
}

SYSCALL_IMPLEMENTATION(CREATE_THREAD)
{
    // TODO
    return ErrorCode::UNSUPPORTED;
}

}
