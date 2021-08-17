cmake_minimum_required(VERSION 3.16)

set(ENV{ULTRA_ARCH} ${ARCH})

project(Kernel)
enable_language(ASM_NASM)
enable_language(CXX)

if (ARCH MATCHES "64")
    set(CMAKE_ASM_NASM_FLAGS "-felf64 -DULTRA_64 -I${PROJECT_SOURCE_DIR}/Architecture/x86/ -I${PROJECT_SOURCE_DIR}/Architecture/x86/AMD64")
    add_definitions(-DULTRA_64)
    set(ARCH_FOLDER "x86/AMD64")
    set(ADDITIONAL_FLAGS "-mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2")
else ()
    set(CMAKE_ASM_NASM_FLAGS "-felf32 -DULTRA_32 -I${PROJECT_SOURCE_DIR}/Architecture/x86/ -I${PROJECT_SOURCE_DIR}/Architecture/x86/i386")
    add_definitions(-DULTRA_32)
    set(ARCH_FOLDER "x86/i386")
    set(ADDITIONAL_FLAGS "-mgeneral-regs-only")
endif ()

# Don't set flags if we're building cmake purely for the sake of syntax highlighting,
# because obviously MSVC doesn't support these flags.
if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-use-cxa-atexit")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdlib")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ADDITIONAL_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-builtin-macro-redefined -D'__FILE__=\"$(subst $(realpath ${CMAKE_SOURCE_DIR})/,,$(abspath $<))\"'")
    set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <FLAGS> -o <OBJECT> <SOURCE>")
endif ()

FILE(GLOB CPP_FILES "${PROJECT_SOURCE_DIR}/*cpp"                  "${PROJECT_SOURCE_DIR}/*h"
                    "${PROJECT_SOURCE_DIR}/Core/*cpp"             "${PROJECT_SOURCE_DIR}/Core/*h"
                    "${PROJECT_SOURCE_DIR}/Common/*cpp"           "${PROJECT_SOURCE_DIR}/Common/*h"
                    "${PROJECT_SOURCE_DIR}/Interrupts/*cpp"       "${PROJECT_SOURCE_DIR}/Interrupts/*h"
                    "${PROJECT_SOURCE_DIR}/Memory/*cpp"           "${PROJECT_SOURCE_DIR}/Memory/*h"
                    "${PROJECT_SOURCE_DIR}/Multitasking/*cpp"     "${PROJECT_SOURCE_DIR}/Multitasking/*h"
                    "${PROJECT_SOURCE_DIR}/Multitasking/ELF/*cpp" "${PROJECT_SOURCE_DIR}/Multitasking/ELF/*h"
                    "${PROJECT_SOURCE_DIR}/Time/*cpp"             "${PROJECT_SOURCE_DIR}/Time/*h"
                    "${PROJECT_SOURCE_DIR}/Drivers/*cpp"          "${PROJECT_SOURCE_DIR}/Drivers/*h"
                    "${PROJECT_SOURCE_DIR}/Drivers/Video/*cpp"    "${PROJECT_SOURCE_DIR}/Drivers/Video/*h"
                    "${PROJECT_SOURCE_DIR}/Drivers/PS2/*cpp"      "${PROJECT_SOURCE_DIR}/Drivers/PS2/*h"
                    "${PROJECT_SOURCE_DIR}/Drivers/PCI/*cpp"      "${PROJECT_SOURCE_DIR}/Drivers/PCI/*h"
                    "${PROJECT_SOURCE_DIR}/Drivers/AHCI/*cpp"     "${PROJECT_SOURCE_DIR}/Drivers/AHCI/*h"
                    "${PROJECT_SOURCE_DIR}/Drivers/USB/*cpp"      "${PROJECT_SOURCE_DIR}/Drivers/USB/*h"
                    "${PROJECT_SOURCE_DIR}/Drivers/USB/XHCI/*cpp" "${PROJECT_SOURCE_DIR}/Drivers/USB/XHCI/*h"
                    "${PROJECT_SOURCE_DIR}/WindowManager/*cpp"    "${PROJECT_SOURCE_DIR}/WindowManager/*h"
                    "${PROJECT_SOURCE_DIR}/ACPI/*cpp"             "${PROJECT_SOURCE_DIR}/ACPI/*h"
                    "${PROJECT_SOURCE_DIR}/FileSystem/*cpp"       "${PROJECT_SOURCE_DIR}/FileSystem/*h"
                    "${PROJECT_SOURCE_DIR}/FileSystem/FAT32/*cpp" "${PROJECT_SOURCE_DIR}/FileSystem/FAT32/*h")

FILE(GLOB ASM_FILES "${PROJECT_SOURCE_DIR}/Architecture/${ARCH_FOLDER}/*asm")

set(KERNEL Kernel.bin PARENT_SCOPE)
set(KERNEL_PATH ${PROJECT_BINARY_DIR} PARENT_SCOPE)
set(KERNEL_SOURCES ${CPP_FILES} ${ASM_FILES})
add_executable(Kernel.bin ${KERNEL_SOURCES})

target_include_directories(Kernel.bin PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/../)
set_target_properties(Kernel.bin PROPERTIES LINK_FLAGS "-T ${PROJECT_SOURCE_DIR}/Architecture/${ARCH_FOLDER}/Linker.ld -Wl,--oformat,binary -Wl,-Map=kernel.map")

target_link_libraries(Kernel.bin gcc)
