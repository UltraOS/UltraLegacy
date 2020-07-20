cmake_minimum_required(VERSION 3.10)

set(ENV{ULTRA_ARCH} ${ARCH})
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../Toolchain/CMakeToolchain.txt)

project(Kernel)
enable_language(ASM_NASM)
enable_language(CXX)

set(CMAKE_CXX_FLAGS         ${CXX_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG   ${CXX_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE ${CXX_FLAGS})
set(CMAKE_EXE_LINKER_FLAGS  "-T ${PROJECT_SOURCE_DIR}/Linker.ld ${LINKER_FLAGS}")

set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <FLAGS> -o <OBJECT> <SOURCE>")

if (ARCH MATCHES "64")
    set(CMAKE_ASM_NASM_FLAGS "-felf64")
else ()
    set(CMAKE_ASM_NASM_FLAGS "-felf32")
endif ()

FILE(GLOB CPP_FILES "${PROJECT_SOURCE_DIR}/*cpp"              "${PROJECT_SOURCE_DIR}/*h"
                    "${PROJECT_SOURCE_DIR}/Core/*cpp"         "${PROJECT_SOURCE_DIR}/Core/*h"
                    "${PROJECT_SOURCE_DIR}/Common/*cpp"       "${PROJECT_SOURCE_DIR}/Common/*h"
                    "${PROJECT_SOURCE_DIR}/Interrupts/*cpp"   "${PROJECT_SOURCE_DIR}/Interrupts/*h"
                    "${PROJECT_SOURCE_DIR}/Memory/*cpp"       "${PROJECT_SOURCE_DIR}/Memory/*h"
                    "${PROJECT_SOURCE_DIR}/Multitasking/*cpp" "${PROJECT_SOURCE_DIR}/Multitasking/*h")

FILE(GLOB ASM_FILES "${PROJECT_SOURCE_DIR}/Core/*asm"
                    "${PROJECT_SOURCE_DIR}/Multitasking/*asm")

set(KERNEL_SOURCES ${CPP_FILES} ${ASM_FILES})

set(KERNEL Kernel.bin PARENT_SCOPE)
set(KERNEL_PATH ${PROJECT_BINARY_DIR} PARENT_SCOPE)

add_executable(Kernel.bin ${KERNEL_SOURCES})
target_include_directories(Kernel.bin PRIVATE "${PROJECT_SOURCE_DIR}")
target_link_libraries(Kernel.bin gcc)