#include "Process.h"
#include "TaskLoader.h"
#include "FileSystem/VFS.h"
#include "Memory/PrivateVirtualRegion.h"
#include "ELF/ELFLoader.h"
#include "Core/FPU.h"
#include "Scheduler.h"

namespace kernel {

#define TERMINATE_WITH_ERROR(code) \
    request->error_code = code;   \
    request->blocker.unblock();    \
    Thread::current()->set_invulnerable(false); \
    Scheduler::the().exit(static_cast<size_t>(code));

void TaskLoader::do_load(LoadRequest* request)
{
    Thread::current()->set_invulnerable(true);

    ASSERT(request != nullptr);
    log() << "TaskLoader: loading a task at " << request->path;

    auto& path = request->path;
    auto& process = Process::current();

    auto error_or_file = VFS::the().open(path, FileMode::READONLY);

    if (error_or_file.is_error()) {
        TERMINATE_WITH_ERROR(error_or_file.error().value);
    }

    auto& fd = error_or_file.value();

    auto res = ELFLoader::load(*fd);
    // TODO: store fd somewhere so we have a refcount?

    if (res.is_error()) {
        TERMINATE_WITH_ERROR(res.error().value);
    }

    request->error_code = ErrorCode::NO_ERROR;
    request->blocker.unblock();

    Interrupts::disable();

    Thread* main_thread;

    {
        LOCK_GUARD(process.lock());
        main_thread = (*process.threads().begin()).get();
    }

    Address userspace_iret_frame = main_thread->kernel_stack().virtual_range().end() - sizeof(RegisterState);
#ifdef ULTRA_32
    userspace_iret_frame.as_pointer<RegisterState>()->eip = res.value();
#elif defined(ULTRA_64)
    userspace_iret_frame.as_pointer<RegisterState>()->rip = res.value();
#endif

    FPU::restore_state(main_thread->fpu_state());

    Thread::current()->set_invulnerable(false);
    jump_to_userspace(userspace_iret_frame);
}

ErrorOr<RefPtr<Process>> TaskLoader::load_from_file(StringView path)
{
    Thread::ScopedInvulnerability si;

    auto* request = new LoadRequest { path, *Thread::current(), ErrorCode::NO_ERROR };
    auto* new_address_space = new AddressSpace;

    // TODO: configure stack size?
    auto process = Process::create_user(path, new_address_space, request);
    Scheduler::the().register_process(process);

    // Block until the process is either loaded or the load terminates with error
    request->blocker.block();

    auto error_code = request->error_code;
    delete request;

    if (error_code)
        return error_code;

    return process;
}


}