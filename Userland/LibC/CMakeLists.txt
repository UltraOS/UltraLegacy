cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../Toolchain/CMakeToolchain.txt)

project(LibC)
enable_language(C)

set(CMAKE_C_FLAGS         ${C_FLAGS})
set(CMAKE_C_FLAGS_DEBUG   ${C_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${C_FLAGS})

FILE(GLOB C_FILES "${PROJECT_SOURCE_DIR}/*c" "${PROJECT_SOURCE_DIR}/*h")
add_library(C STATIC ${C_FILES})
target_link_libraries(C PRIVATE gcc)