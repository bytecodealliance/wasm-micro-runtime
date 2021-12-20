# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# from ESP-IDF 4.0 examples/build_system/cmake/idf_as_lib
cmake_minimum_required(VERSION 3.5)
project(wamr_on_esp32c3)

enable_language(ASM)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

if("${IDF_TARGET}" STREQUAL "")
    message(FATAL_ERROR "You need to set IDF_TARGET to your target string")
endif()

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
# Create idf::esp32c3 and idf::freertos static libraries
idf_build_process(${IDF_TARGET}
                # try and trim the build; additional components
                # will be included as needed based on dependency tree
                #
                # although esptool_py does not generate static library,
                # processing the component is needed for flashing related
                # targets and file generation
                COMPONENTS ${IDF_TARGET} freertos esptool_py
                SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig
                BUILD_DIR ${CMAKE_BINARY_DIR})

# Set WAMR's build options
if("${IDF_TARGET}" STREQUAL "esp32c3")
    set(WAMR_BUILD_TARGET "RISCV32")
else()
    set(WAMR_BUILD_TARGET "XTENSA")
    add_compile_options(-DWAMR_BUILD_TARGET_XTENSA=1)
endif()

set(WAMR_BUILD_PLATFORM "esp-idf")

if (NOT DEFINED WAMR_BUILD_INTERP)
  set (WAMR_BUILD_INTERP 0)
endif ()

if (NOT DEFINED WAMR_BUILD_FAST_INTERP)
  set (WAMR_BUILD_FAST_INTERP 0)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  set (WAMR_BUILD_AOT 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_APP_FRAMEWORK)
  set (WAMR_BUILD_APP_FRAMEWORK 0)
endif ()


# Set the compile time variable so that the right binary is selected
add_compile_options(-DWAMR_BUILD_INTERP=${WAMR_BUILD_INTERP})

set(WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

# define WAMR as library and provide it the esp-idf srcs
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
target_link_libraries(vmlib PUBLIC idf::pthread idf::${IDF_TARGET} idf::freertos)

# Define the final executable
set(elf_file ${CMAKE_PROJECT_NAME}.elf)
add_executable(${elf_file} main.c test_wasm.h)
target_link_libraries(${elf_file} idf::${IDF_TARGET} idf::freertos idf::spi_flash vmlib)
idf_build_executable(${elf_file})
