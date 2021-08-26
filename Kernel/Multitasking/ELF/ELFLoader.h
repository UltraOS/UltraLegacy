#pragma once

#include "Common/Macros.h"
#include "Core/ErrorCode.h"
#include "FileSystem/IOStream.h"
#include "Structures.h"

namespace kernel {

class ELFLoader {
    MAKE_STATIC(ELFLoader);

public:
    static ErrorOr<Address> load(IOStream&);

private:
    static bool validate_header(const ELF::Header&);
};

}
