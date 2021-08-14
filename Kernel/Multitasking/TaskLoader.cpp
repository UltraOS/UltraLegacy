#include "Process.h"
#include "TaskLoader.h"
#include "FileSystem/VFS.h"
#include "Memory/PrivateVirtualRegion.h"
#include "ELF/ELFLoader.h"
#include "Core/FPU.h"
#include "Scheduler.h"

namespace kernel {

#define TERMINATE_WITH_ERROR(code)                   \
    request->error_code = code;                      \
    if (request->blocker)                            \
        request->blocker->unblock();                 \
    Thread::current()->set_invulnerable(false);      \
    Scheduler::the().exit_process(static_cast<size_t>(code));

// NOTE: This function is noreturn, so don't create any non-trivially destructible objects
//       on the stack in the main scope, their destructors will never be called.
void TaskLoader::do_load(LoadRequest* request)
{
    Thread::current()->set_invulnerable(true);

    ASSERT(request != nullptr);
    log() << "TaskLoader: loading a task at " << request->path;

    auto& path = request->path;
    auto& process = Process::current();

    Address userspace_entrypoint;
    {
        auto error_or_file = VFS::the().open(path, IOMode::READONLY);

        if (error_or_file.is_error()) {
            TERMINATE_WITH_ERROR(error_or_file.error().value);
        }

        auto& file = error_or_file.value();

        auto res = ELFLoader::load(*file);
        file->close();

        if (res.is_error()) {
            TERMINATE_WITH_ERROR(res.error().value);
        }

        userspace_entrypoint = res.value();
    }

    Address stack_begin;
    {
        auto stack_region = MemoryManager::the().allocate_user_stack("base stack"_sv, AddressSpace::current());
        stack_begin = stack_region->virtual_range().end();
        process.store_region(stack_region);

        static constexpr size_t preallocated_size = 32 * KB;

        // Preallocate a bit of stack so we avoid a few potential page faults in the beginning, which are very likely.
        auto preallocated_begin = stack_begin - preallocated_size;
        static_cast<PrivateVirtualRegion*>(stack_region.get())->preallocate_specific({ preallocated_begin, preallocated_size });
    }

    if (!request->work_dir.empty())
        process.set_working_directory(request->work_dir);

    if (request->stdin_stream)
        process.store_io_stream_at(0, request->stdin_stream);
    if (request->stdout_stream)
        process.store_io_stream_at(1, request->stdout_stream);
    if (request->stderr_stream)
        process.store_io_stream_at(2, request->stderr_stream);

    auto current_sp = stack_begin;

    auto push_str_on_the_stack = [&current_sp] (StringView str)
    {
        current_sp -= str.size() + 1;
        copy_memory(str.data(), current_sp.as_pointer<void>(), str.size());
        current_sp.as_pointer<char>()[str.size()] = '\0';
    };

    auto push_ptr_on_the_stack = [&current_sp] (Address addr)
    {
        current_sp -= sizeof(Address::underlying_pointer_type);
        *current_sp.as_pointer<Address::underlying_pointer_type>() = addr;
    };

    {
        DynamicArray<Address> user_envp;
        user_envp.reserve(request->envp.size());

        for (size_t i = request->envp.size(); i-- > 0;) {
            push_str_on_the_stack(request->envp[i].to_view());
            user_envp.emplace(current_sp);
        }

        DynamicArray<Address> user_argv;
        user_argv.reserve(request->argv.size());

        for (size_t i = request->argv.size(); i-- > 0;) {
            push_str_on_the_stack(request->argv[i].to_view());
            user_argv.emplace(current_sp);
        }

        current_sp -= current_sp % 16;

        static constexpr size_t words_per_16_bytes = 16 / sizeof(ptr_t);

        // misalign stack to N to keep 16 byte alignment after push
        auto remainder = (user_envp.size() + user_argv.size()) % words_per_16_bytes;
        if (remainder)
            current_sp -= (words_per_16_bytes - remainder) * sizeof(ptr_t);

        // null aux vector entry
        push_ptr_on_the_stack(nullptr);

        // end of envp
        push_ptr_on_the_stack(nullptr);

        for (auto& addr : user_envp)
            push_ptr_on_the_stack(addr);

        // end of argv
        push_ptr_on_the_stack(nullptr);

        for (auto& addr : user_argv)
            push_ptr_on_the_stack(addr);

        // argc
        push_ptr_on_the_stack(user_argv.size());
    }

    ASSERT((current_sp % 16) == 0);

    request->error_code = ErrorCode::NO_ERROR;

    if (request->blocker)
        request->blocker->unblock();

    Interrupts::disable();

    Thread* main_thread;

    {
        LOCK_GUARD(process.lock());
        main_thread = (*process.threads().begin()).get();
    }

    Address userspace_iret_frame = main_thread->kernel_stack().virtual_range().end() - sizeof(RegisterState);
    auto reg_state = userspace_iret_frame.as_pointer<RegisterState>();
    reg_state->set_instruction_pointer(userspace_entrypoint);
    reg_state->set_userspace_stack_pointer(current_sp);

    FPU::restore_state(main_thread->fpu_state());

    Thread::current()->set_invulnerable(false);
    jump_to_userspace(userspace_iret_frame);
}

ErrorOr<RefPtr<Process>> TaskLoader::load_from_file(LoadParameters& params)
{
    Thread::ScopedInvulnerability si;

    ProcessLoadBlocker blocker(*Thread::current());

    auto request = LoadRequest(move(params));
    request.blocker = &blocker;

    auto* new_address_space = new AddressSpace;

    // TODO: configure stack size?
    auto process = Process::create_user(params.path, new_address_space, &request);
    Scheduler::the().register_process(process);

    // Block until the process is either loaded or the load terminates with error
    blocker.block();

    if (request.error_code)
        return request.error_code;

    return process;
}


}
