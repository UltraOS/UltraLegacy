#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"

namespace kernel::ELF  {

enum class Class {
    NONE = 0,
    ELF32 = 1,
    ELF64 = 2
};

enum class DataEncoding {
    NONE = 0,
    LITTLE_ENDIAN = 1,
    BIG_ENDIAN = 2
};

enum class Type : u16 {
    NONE = 0,
    RELOCATABLE = 1,
    EXECUTABLE = 2,
    DYNAMIC = 3,
    CORE = 4,
};

enum class Machine : u16 {
    NONE = 0,
    SPARC = 2,
    I386 = 3,
    SPARC32PLUS = 18,
    SPARCV9 = 43,
    AMD64 = 62,
};

struct PACKED Header {
    u8 magic[4];
    Class file_class;
    DataEncoding encoding;
    u8 ident_version;
    u8 os_abi;
    u8 abi_version;
    u8 padding[7];
    Type type;
    Machine machine;
    u32 version;
    ptr_t entrypoint;
    size_t program_headers_offset;
    size_t section_headers_offset;
    u32 flags;
    u16 header_size;
    u16 bytes_per_program_header;
    u16 program_header_count;
    u16 bytes_per_section_header;
    u16 section_header_count;
    u16 section_name_table_index;
};

struct PACKED ProgramHeader {
    enum class Type : u32 {
        IGNORE = 0,
        LOAD = 1,
        DYNAMIC = 2,
        INTERPRETER = 3,
        NOTE = 4,
        SHLIB = 5,
        PROGRAM_HEADER_TABLE = 6,
        THREAD_LOCAL_STORAGE = 7,
    } type;

    enum class Flags : u32 {
        EXECUTE = 1,
        WRITE = 2,
        READ = 4,
    };

#ifdef ULTRA_32
    size_t offset;
    ptr_t virtual_address;
    ptr_t physical_address;
    u32 bytes_in_file;
    u32 bytes_in_memory;
    Flags flags;
    u32 alignment;
#elif defined(ULTRA_64)
    Flags flags;
    size_t offset_in_file;
    ptr_t virtual_address;
    ptr_t physical_address;
    u64 bytes_in_file;
    u64 bytes_in_memory;
    u64 alignment;
#endif
};

}