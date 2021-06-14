#pragma once

#include "Common/RefPtr.h"
#include "Core/ErrorCode.h"
#include "Blocker.h"

namespace kernel {

class Process;

class TaskLoader {
public:
    static ErrorOr<RefPtr<Process>> load_from_file(StringView path);

    struct LoadRequest {
        StringView path;
        ProcessLoadBlocker blocker;
        ErrorCode error_code;
    };

    static void do_load(LoadRequest*);

private:
    static void jump_to_userspace(ptr_t stack_pointer);
};

}