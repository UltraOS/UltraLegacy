#pragma once

#include "Common/Macros.h"
#include "Core/ErrorCode.h"
#include "FileSystem/FileDescription.h"
#include "Structures.h"

namespace kernel {

class ELFLoader {
    MAKE_STATIC(ELFLoader);
public:
    static ErrorOr<Address> load(FileDescription& fd);

private:
    static bool validate_header(const ELF::Header&);
};

}