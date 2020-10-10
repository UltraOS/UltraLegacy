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

if (NOT ARCH)
    set(ARCH "32")
elseif ((NOT ARCH MATCHES "32") AND (NOT ARCH MATCHES "64"))
    message(FATAL_ERROR "Unknown architechture ${ARCH}")
endif ()

set(ENV{ULTRA_ARCH} ${ARCH})

if (NOT WIN32)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/Toolchain/CMakeToolchain.txt)
else()
    add_compile_options("/std:c++latest")
endif(NOT WIN32)

project(UltraOS)

add_subdirectory(Boot)
add_subdirectory(Kernel)

add_custom_target(DISK_IMAGE ALL DEPENDS ${PROJECT_SOURCE_DIR}/Images/Ultra${ARCH}HDD.vmdk)
add_custom_target(KSYMS ALL DEPENDS ${KERNEL_PATH}/KSyms.map)

add_custom_command(
    OUTPUT ${KERNEL_PATH}/KSyms.map
    COMMAND python3 ${PROJECT_SOURCE_DIR}/Scripts/generate_ksyms_map.py ${KERNEL_PATH} ${ELF_KERNEL}
    DEPENDS ${ELF_KERNEL}
)

string(CONCAT VHC_BUILD_COMMAND
  "mkdir -p ${PROJECT_SOURCE_DIR}/Images && "
  "${CMAKE_SOURCE_DIR}/Scripts/vhc --mbr ${BOOTLOADER_PATH}/${MBR} "
                                  "--vbr ${BOOTLOADER_PATH}/${VBR} "
                                  "--files ${BOOTLOADER_PATH}/${KERNEL_LOADER} ${KERNEL_PATH}/${KERNEL} ${KERNEL_PATH}/KSyms.map "
                                  "--size 64 --image-dir ${PROJECT_SOURCE_DIR}/Images --image-name Ultra${ARCH}HDD"
)

separate_arguments(VHC_BUILD_COMMAND UNIX_COMMAND "${VHC_BUILD_COMMAND}")

add_custom_command(
    OUTPUT   ${PROJECT_SOURCE_DIR}/Images/Ultra${ARCH}HDD.vmdk
    COMMAND  ${VHC_BUILD_COMMAND}
    DEPENDS  ${KERNEL} ${ELF_KERNEL} ${KERNEL_PATH}/KSyms.map ${MBR} ${VBR} ${KERNEL_LOADER}
)

