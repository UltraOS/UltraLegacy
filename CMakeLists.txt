cmake_minimum_required(VERSION 3.16)

if (NOT ARCH)
    set(ARCH "64")
elseif ((NOT ARCH MATCHES "32") AND (NOT ARCH MATCHES "64"))
    message(FATAL_ERROR "Unknown architechture ${ARCH}")
endif ()

if (ARCH MATCHES "64")
  set(COMPILER_PREFIX "x86_64")
else ()
  set(COMPILER_PREFIX "i686")
endif ()

if (NOT WIN32)
    execute_process(
        COMMAND ${CMAKE_SOURCE_DIR}/Scripts/build_vhc.sh
        RESULT_VARIABLE VHC_BUILD_FAILED
    )

    if (VHC_BUILD_FAILED)
        message(FATAL_ERROR "-- VHC build error.")
    endif()
endif(NOT WIN32)

execute_process(
    COMMAND /bin/bash ${CMAKE_SOURCE_DIR}/Toolchain/build.sh ${ARCH}
    RESULT_VARIABLE TOOLCHAIN_BUILD_FAILED
)

if (NOT WIN32)
    if (TOOLCHAIN_BUILD_FAILED)
        message(FATAL_ERROR "-- Toolchain build error.")
    endif()

    # dirty hack because it tries to compile with -lc and crt0.o
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER_WORKS 1)

    set(TOOLCHAIN_ROOT ${CMAKE_SOURCE_DIR}/Toolchain/CrossCompiler/Tools${ARCH})
    set(TOOLCHAIN_PATH ${TOOLCHAIN_ROOT}/bin)
    set(PATH_TO_FILT "${TOOLCHAIN_PATH}/${COMPILER_PREFIX}-pc-ultra-c++filt")
    set(TOOLCHAIN_PREFIX ${TOOLCHAIN_PATH}/${COMPILER_PREFIX}-pc-ultra-)

    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
    set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
    set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}ld)
    set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}gcc-ranlib)
    set(CMAKE_STRIP ${TOOLCHAIN_PREFIX}strip)
    set(CMAKE_AR ${TOOLCHAIN_PREFIX}gcc-ar)
endif ()

project(UltraOS C CXX ASM)

set(ENV{ULTRA_ARCH} ${ARCH})

if (WIN32)
    add_compile_options("/std:c++latest")
endif (WIN32)

add_subdirectory(Boot)
add_subdirectory(Kernel)
add_subdirectory(Userland)
add_subdirectory(Ports)

add_custom_target(DISK_IMAGE ALL DEPENDS ${PROJECT_SOURCE_DIR}/Images/Ultra${ARCH}HDD.vmdk)

add_custom_command(
    OUTPUT ${KERNEL_PATH}/KSyms.map
    COMMAND python3 ${PROJECT_SOURCE_DIR}/Scripts/generate_ksyms_map.py ${KERNEL_PATH} kernel.map ${PATH_TO_FILT}
    DEPENDS ${KERNEL}
)

message("Detected port files: ${PORT_FILES_LIST}, executables: ${PORT_EXECUTABLES}")

string(CONCAT VHC_BUILD_COMMAND
  "mkdir -p ${PROJECT_SOURCE_DIR}/Images && "
  "${CMAKE_SOURCE_DIR}/Scripts/vhc --mbr ${BOOTLOADER_PATH}/${MBR} "
                                  "--filesystem FAT32,vfat=false,vbr=${BOOTLOADER_PATH}/${VBR} "
                                  "--files ${BOOTLOADER_PATH}/${KERNEL_LOADER} ${KERNEL_PATH}/${KERNEL} ${KERNEL_PATH}/KSyms.map ${PORT_FILES_LIST} "
                                  "--size 64 --image-directory ${PROJECT_SOURCE_DIR}/Images --image-name Ultra${ARCH}HDD"
)

separate_arguments(VHC_BUILD_COMMAND UNIX_COMMAND "${VHC_BUILD_COMMAND}")
add_custom_command(
    OUTPUT   ${PROJECT_SOURCE_DIR}/Images/Ultra${ARCH}HDD.vmdk
    COMMAND  ${VHC_BUILD_COMMAND}
    DEPENDS  ${KERNEL} ${KERNEL_PATH}/KSyms.map ${MBR} ${VBR} ${KERNEL_LOADER} ${PORT_EXECUTABLES}
)
