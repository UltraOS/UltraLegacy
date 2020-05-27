cmake_minimum_required(VERSION 3.10)

project(Bootloader)

enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

# A workaround to make cmake not try to link our flat binary file
set(CMAKE_ASM_NASM_LINK_EXECUTABLE "cp <OBJECTS> <TARGET>")

# This allows us to produce flat binary files as opposed to elf
add_compile_options(
    "$<$<COMPILE_LANGUAGE:ASM_NASM>:-f $<IF:$<BOOL:$<TARGET_PROPERTY:NASM_OBJ_FORMAT>>, \
    $<TARGET_PROPERTY:NASM_OBJ_FORMAT>, ${CMAKE_ASM_NASM_OBJECT_FORMAT}>>"
)

set(BOOTLOADER_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
file(GLOB BOOTLOADER_INCLUDES "${BOOTLOADER_INCLUDE_DIR}/*.inc")

# Using include_directories() produces a NASM-incompatible
# include path, as nasm requires a slash after the last directory
set(CMAKE_ASM_NASM_FLAGS "-I${BOOTLOADER_INCLUDE_DIR}/")

add_executable(UltraMBR.bin  UltraMBR.asm)
add_executable(UltraBoot.bin UltraBoot.asm)
add_executable(UKLoader.bin  UKLoader.asm)

set(MBR           UltraMBR.bin  PARENT_SCOPE)
set(VBR           UltraBoot.bin PARENT_SCOPE)
set(KERNEL_LOADER UKLoader.bin  PARENT_SCOPE)
set(BOOTLOADER_PATH ${PROJECT_BINARY_DIR} PARENT_SCOPE)

# Make the bootloader files regenerate if we change headers
foreach(INCLUDE_FILE ${BOOTLOADER_INCLUDES})
    set_property(SOURCE UltraMBR.asm  APPEND PROPERTY OBJECT_DEPENDS ${INCLUDE_FILE})
    set_property(SOURCE UltraBoot.asm APPEND PROPERTY OBJECT_DEPENDS ${INCLUDE_FILE})
    set_property(SOURCE UKLoader.asm  APPEND PROPERTY OBJECT_DEPENDS ${INCLUDE_FILE})
endforeach()

set_target_properties(UltraMBR.bin  PROPERTIES NASM_OBJ_FORMAT bin)
set_target_properties(UltraBoot.bin PROPERTIES NASM_OBJ_FORMAT bin)
set_target_properties(UKLoader.bin  PROPERTIES NASM_OBJ_FORMAT bin)
