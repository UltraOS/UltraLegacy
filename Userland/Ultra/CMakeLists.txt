cmake_minimum_required(VERSION 3.10)

project(Ultra)
enable_language(C)

if (NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
endif (NOT WIN32)

FILE(GLOB C_FILES "${PROJECT_SOURCE_DIR}/*c" "${PROJECT_SOURCE_DIR}/*h")
add_library(Ultra STATIC ${C_FILES})
target_include_directories(Ultra PUBLIC ${PROJECT_SOURCE_DIR}/../../)
target_link_libraries(Ultra PRIVATE gcc)