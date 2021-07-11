cmake_minimum_required(VERSION 3.16)

set(ENV{ULTRA_ARCH} ${ARCH})
project(LibC C)

enable_language(ASM_NASM)
enable_language(C)

if (ARCH MATCHES "64")
    set(CMAKE_ASM_NASM_FLAGS "-felf64 -DULTRA_64")
    add_definitions(-DULTRA_64)
else ()
    set(CMAKE_ASM_NASM_FLAGS "-felf32 -DULTRA_32")
    add_definitions(-DULTRA_32)
endif ()

set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <FLAGS> -o <OBJECT> <SOURCE>")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")

FILE(GLOB C_FILES "${PROJECT_SOURCE_DIR}/*c" "${PROJECT_SOURCE_DIR}/Ultra/*c" "${PROJECT_SOURCE_DIR}/*h")

set(USERLAND_PATH "${PROJECT_SOURCE_DIR}/../")
set(ROOT_PATH "${USERLAND_PATH}../")
set(SYSROOT_PATH "${ROOT_PATH}/Root${ARCH}")
set(LIBRARIES_PATH "${SYSROOT_PATH}/System/Libraries")
set(INCLUDES_PATH "${SYSROOT_PATH}/System/Includes")

add_library(crt0 STATIC crt0.c)
add_custom_command(
    TARGET crt0
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_OBJECTS:crt0> ${LIBRARIES_PATH}/crt0.o)

add_library(crti STATIC crti.asm)
add_custom_command(
    TARGET crti
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_OBJECTS:crti> ${LIBRARIES_PATH}/crti.o)

add_library(crtn STATIC crtn.asm)
add_custom_command(
    TARGET crtn
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_OBJECTS:crtn> ${LIBRARIES_PATH}/crtn.o)

add_library(c STATIC ${C_FILES})
target_include_directories(c PUBLIC "${USERLAND_PATH}" "${ROOT_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(c ultra)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.
    DESTINATION ${INCLUDES_PATH}
    FILES_MATCHING PATTERN "*.h")

install(
    DIRECTORY ${ROOT_PATH}/Shared
    DESTINATION ${INCLUDES_PATH}
    FILES_MATCHING PATTERN "*.h")

install(
    TARGETS c
    DESTINATION ${LIBRARIES_PATH}
)

add_custom_command(
    TARGET c POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR} --config $<CONFIG>
)
