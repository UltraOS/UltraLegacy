#include "Structures.h"
#include "ELFLoader.h"

#define ELF_LOG log("ELFLoader")

#ifdef ELF_DEBUG_MODE
#define ELF_DEBUG log("ELFLoader")
#else
#define ELF_DEBUG DummyLogger()
#endif

namespace kernel {

bool ELFLoader::validate_header(const ELF::Header& header)
{
    static const u8 magic[4] = { 0x7F, 'E', 'L', 'F' };

    if (!compare_memory(magic, header.magic, sizeof(magic))) {
        ELF_LOG << "file has invalid ELF header";
        return false;
    }

#ifdef ULTRA_32
    ELF::Class expected_class = ELF::Class::ELF32;
    ELF::Machine expected_machine = ELF::Machine::I386;
#elif defined(ULTRA_64)
    ELF::Class expected_class = ELF::Class::ELF64;
    ELF::Machine expected_machine = ELF::Machine::AMD64;
#endif
    ELF::Type expected_type = ELF::Type::EXECUTABLE;

    if (header.file_class != expected_class) {
        ELF_LOG << "file has invalid class " << static_cast<u8>(header.file_class)
                << " vs expected " << static_cast<u8>(expected_class);

        return false;
    }

    if (header.encoding != ELF::DataEncoding::LITTLE_ENDIAN) {
        ELF_LOG << "expected a little endian elf, got encoding "
                << static_cast<u8>(header.encoding);

        return false;
    }

    if (header.type != expected_type) {
        ELF_LOG << "expected executable elf, got type " << static_cast<u8>(header.type);
        return false;
    }

    if (header.machine != expected_machine) {
        ELF_LOG << "unexpected elf machine type " << static_cast<u8>(header.machine);
        return false;
    }

    return true;
}

ErrorOr<Address> ELFLoader::load(FileDescription& fd)
{
    ELF::Header header {};

    if (fd.read(&header, sizeof(header)) != sizeof(header))
        return ErrorCode::INVALID_ARGUMENT;

    if (!validate_header(header))
        return ErrorCode::INVALID_ARGUMENT;

    return ErrorCode::UNSUPPORTED;
}


}