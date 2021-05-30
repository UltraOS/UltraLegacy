cmake_minimum_required(VERSION 3.16)

project(LibC C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")

FILE(GLOB C_FILES "${PROJECT_SOURCE_DIR}/*c" "${PROJECT_SOURCE_DIR}/*h")

add_library(c STATIC ${C_FILES})
target_include_directories(c PUBLIC "${PROJECT_SOURCE_DIR}/.." "${PROJECT_SOURCE_DIR}/../../")
target_link_libraries(c gcc)
