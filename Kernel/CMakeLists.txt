cmake_minimum_required(VERSION 3.10)

set(LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/Linker.ld -Wl,--oformat=binary -ffreestanding -O2 -nostdlib")
set(CXX_FLAGS "-std=c++17 -ffreestanding -O2 -Wall -Wextra -fno-use-cxa-atexit -fno-rtti -fno-exceptions")

set(TOOLCHAIN_PATH              "${CMAKE_SOURCE_DIR}/../Toolchain")
set(CMAKE_C_COMPILER            "${TOOLCHAIN_PATH}/CrossCompiler/Tools/bin/i686-elf-gcc")
set(CMAKE_CXX_COMPILER          "${TOOLCHAIN_PATH}/CrossCompiler/Tools/bin/i686-elf-g++")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${LINKER_FLAGS}")

project(Kernel)
enable_language(ASM_NASM)
enable_language(CXX)

set(CMAKE_CXX_FLAGS         ${CXX_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG   ${CXX_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE ${CXX_FLAGS})
set(CMAKE_EXE_LINKER_FLAGS  ${LINKER_FLAGS})

set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <FLAGS> -o <OBJECT> <SOURCE>")
set(CMAKE_ASM_NASM_FLAGS "-felf32")

FILE(GLOB CPP_FILES "${PROJECT_SOURCE_DIR}/*cpp")
FILE(GLOB ASM_FILES "${PROJECT_SOURCE_DIR}/*asm")
set(SOURCES ${CPP_FILES} ${ASM_FILES})

add_executable(Kernel.bin ${SOURCES})
target_link_libraries(Kernel.bin gcc)