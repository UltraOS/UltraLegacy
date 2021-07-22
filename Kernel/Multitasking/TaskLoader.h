#pragma once

#include "Common/RefPtr.h"
#include "Core/ErrorCode.h"
#include "Blocker.h"
#include "FileSystem/IOStream.h"

namespace kernel {

class Process;

class TaskLoader {
public:
    struct LoadParameters {
        StringView path;
        DynamicArray<String> argv;
        DynamicArray<String> envp;
        StringView work_dir = "/"_sv;
        RefPtr<IOStream> stdin_stream;
        RefPtr<IOStream> stdout_stream;
        RefPtr<IOStream> stderr_stream;
    };

    static ErrorOr<RefPtr<Process>> load_from_file(LoadParameters&);

    struct LoadRequest : public LoadParameters {
        LoadRequest(LoadParameters&& params)
            : LoadParameters(move(params))
        {
        }

        ProcessLoadBlocker* blocker { nullptr };
        ErrorCode error_code { ErrorCode::NO_ERROR };
    };

    static void do_load(LoadRequest*);

private:
    static void jump_to_userspace(ptr_t stack_pointer);
};

}
