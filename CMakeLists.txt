execute_process(
    COMMAND ${CMAKE_SOURCE_DIR}/Scripts/build_vhc.sh
    RESULT_VARIABLE VHC_BUILD_FAILED
)

if (NOT WIN32)
    if (VHC_BUILD_FAILED)
        message(FATAL_ERROR "-- VHC build error.")
    endif()
endif(NOT WIN32)

cmake_minimum_required(VERSION 3.10)

set(ENV{ULTRA_ARCH} ${ARCH})

if (NOT WIN32)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/Toolchain/CMakeToolchain.txt)
else()
    add_compile_options("/std:c++latest")
endif(NOT WIN32)

project(UltraOS)

add_subdirectory(Boot)
add_subdirectory(Kernel)

add_custom_target(DiskImage ALL DEPENDS ${PROJECT_SOURCE_DIR}/Images/UltraHDD.vmdk)

string(CONCAT VHC_BUILD_COMMAND
  "mkdir -p ${PROJECT_SOURCE_DIR}/Images && "
  "${CMAKE_SOURCE_DIR}/Scripts/vhc --mbr ${BOOTLOADER_PATH}/${MBR} "
                                  "--vbr ${BOOTLOADER_PATH}/${VBR} "
                                  "--files ${BOOTLOADER_PATH}/${KERNEL_LOADER} ${KERNEL_PATH}/${KERNEL} "
                                  "--size 64 --image-dir ${PROJECT_SOURCE_DIR}/Images --image-name UltraHDD"
)

separate_arguments(VHC_BUILD_COMMAND UNIX_COMMAND "${VHC_BUILD_COMMAND}")

add_custom_command(
    OUTPUT   ${PROJECT_SOURCE_DIR}/Images/UltraHDD.vmdk
    COMMAND  ${VHC_BUILD_COMMAND}
    DEPENDS  ${KERNEL} ${MBR} ${VBR} ${KERNEL_LOADER}
)

