#include "ELFLoader.h"
#include "Memory/MemoryManager.h"
#include "Structures.h"

#define ELF_LOG log("ELFLoader")

#define ELF_DEBUG_MODE

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

ErrorOr<Address> ELFLoader::load(IOStream& stream)
{
    ELF::Header header {};

    // TODO: more error handling and sanity checking
    if (stream.read(&header, sizeof(header)).value() != sizeof(header))
        return ErrorCode::INVALID_ARGUMENT;

    if (!validate_header(header))
        return ErrorCode::INVALID_ARGUMENT;

    if (header.program_header_count == 0)
        return ErrorCode::INVALID_ARGUMENT;

    if (header.bytes_per_program_header != sizeof(ELF::ProgramHeader)) {
        ELF_LOG << "invalid program header size " << header.bytes_per_program_header
                << ", expected " << sizeof(ELF::ProgramHeader);
        return ErrorCode::INVALID_ARGUMENT;
    }

    size_t current_offset = 0;
    stream.seek(header.program_headers_offset, SeekMode::BEGINNING);

    ELF::ProgramHeader ph {};

    for (size_t i = 0; i < header.program_header_count; ++i) {
        if (stream.read(&ph, sizeof(ph)).value() != sizeof(ph)) {
            ELF_LOG << "failed to read program headers";
            return ErrorCode::INVALID_ARGUMENT;
        }

        if (ph.type != ELF::ProgramHeader::Type::LOAD)
            continue;

        // TODO: memory flags

        current_offset = stream.seek(0, SeekMode::CURRENT).value();

        auto begin = Page::round_down(ph.virtual_address);
        auto end = Page::round_up(begin + ph.bytes_in_memory);
        auto offset = ph.virtual_address - begin;

        ELF_DEBUG << "Program header: at virtual " << format::as_hex << ph.virtual_address
                  << ", " << ph.bytes_in_memory << " memory bytes, " << ph.bytes_in_file
                  << " file bytes, aligned at " << begin << " -> " << end << " with offset " << offset;

        auto region = MemoryManager::the().allocate_user_private("code", Range::from_two_pointers(begin, end));
        static_cast<PrivateVirtualRegion*>(region.get())->preallocate_entire();
        Process::current().store_region(region);

        stream.seek(ph.offset_in_file, SeekMode::BEGINNING);

        stream.read(Address(region->virtual_range().begin() + offset).as_pointer<void>(), ph.bytes_in_file);

        // restore old offset to keep reading program headers
        stream.seek(current_offset, SeekMode::BEGINNING);
    }

    return Address(header.entrypoint);
}

}
