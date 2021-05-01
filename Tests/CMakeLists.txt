cmake_minimum_required(VERSION 3.6)

project (UltraOSTests)

set(CMAKE_BUILD_TYPE Debug)

set(KERNEL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Kernel")
set(OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set_property(GLOBAL PROPERTY EXTERNAL_KERNEL_FILES)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Environment
)

function(KERNEL_FILE)
    cmake_parse_arguments(
            ARG
            ""
            "PATH;FILE"
            ""
            ${ARGN}
    )

    get_property(EXTERNAL_KERNEL_FILES_LOCAL GLOBAL PROPERTY EXTERNAL_KERNEL_FILES)
    list(APPEND EXTERNAL_KERNEL_FILES_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/Environment/${ARG_PATH}/${ARG_FILE}")
    set_property(GLOBAL PROPERTY EXTERNAL_KERNEL_FILES ${EXTERNAL_KERNEL_FILES_LOCAL})

    add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/Environment/${ARG_PATH}/${ARG_FILE}"
    COMMAND ${CMAKE_COMMAND} -E copy
            ${KERNEL_ROOT_DIR}/${ARG_PATH}/${ARG_FILE}
            ${CMAKE_CURRENT_SOURCE_DIR}/Environment/${ARG_PATH})
endfunction(KERNEL_FILE)

KERNEL_FILE(PATH "Memory" FILE "HeapAllocator.cpp")
KERNEL_FILE(PATH "Memory" FILE "HeapAllocator.h")
KERNEL_FILE(PATH "Memory" FILE "VirtualAllocator.cpp")
KERNEL_FILE(PATH "Memory" FILE "VirtualAllocator.h")
KERNEL_FILE(PATH "Memory" FILE "BootAllocator.cpp")
KERNEL_FILE(PATH "Memory" FILE "BootAllocator.h")
KERNEL_FILE(PATH "Memory" FILE "Page.h")
KERNEL_FILE(PATH "Memory" FILE "Range.h")
KERNEL_FILE(PATH "Memory" FILE "MemoryMap.h")
KERNEL_FILE(PATH "Memory" FILE "MemoryMap.cpp")
KERNEL_FILE(PATH "Memory" FILE "Utilities.cpp")
KERNEL_FILE(PATH "Memory" FILE "Utilities.h")
KERNEL_FILE(PATH "Common" FILE "Types.h")
KERNEL_FILE(PATH "Common" FILE "DynamicArray.h")
KERNEL_FILE(PATH "Common" FILE "DynamicBitArray.h")
KERNEL_FILE(PATH "Common" FILE "StaticArray.h")
KERNEL_FILE(PATH "Common" FILE "String.h")
KERNEL_FILE(PATH "Common" FILE "Memory.h")
KERNEL_FILE(PATH "Common" FILE "Math.h")
KERNEL_FILE(PATH "Common" FILE "Traits.h")
KERNEL_FILE(PATH "Common" FILE "Utilities.h")
KERNEL_FILE(PATH "Common" FILE "Conversions.h")
KERNEL_FILE(PATH "Common" FILE "List.h")
KERNEL_FILE(PATH "Common" FILE "RedBlackTree.h")
KERNEL_FILE(PATH "Common" FILE "Set.h")
KERNEL_FILE(PATH "Common" FILE "RefPtr.h")
KERNEL_FILE(PATH "Common" FILE "UniquePtr.h")
KERNEL_FILE(PATH "Common" FILE "Pair.h")
KERNEL_FILE(PATH "Common" FILE "Optional.h")
KERNEL_FILE(PATH "Common" FILE "CircularBuffer.h")
KERNEL_FILE(PATH "Core"   FILE "Boot.h")

KERNEL_FILE(PATH "FileSystem" FILE "Utilities.h")
KERNEL_FILE(PATH "FileSystem/FAT32" FILE "Utilities.h")
KERNEL_FILE(PATH "Drivers/Video" FILE "VideoMode.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY         "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

get_property(EXTERNAL_KERNEL_FILES_LOCAL GLOBAL PROPERTY EXTERNAL_KERNEL_FILES)
FILE(GLOB_RECURSE SORUCE_FILES "${SOURCE_DIR}/*cpp" "${SOURCE_DIR}/*h")

add_custom_target(ExternalFiles DEPENDS ${EXTERNAL_KERNEL_FILES_LOCAL})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DULTRA_64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-DULTRA_32)
endif()

add_executable(RunTests ${SORUCE_FILES})
add_dependencies(RunTests ExternalFiles)

target_compile_features(RunTests PRIVATE cxx_std_17)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RunTests)
