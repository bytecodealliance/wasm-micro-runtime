# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)
project(linking_samples)

set(CMAKE_BUILD_TYPE Debug)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(WASISDK REQUIRED)

include(CTest)
enable_testing()

################  runtime settings  ################
string (TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
include(CheckPIESupported)

# AOT and JIT byd default
if(NOT WAMR_BUILD_AOT)
  set(WAMR_BUILD_AOT 0)
endif()
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_JIT 0)
# wasm32-wasi
set(WAMR_BUILD_LIBC_BUILTIN 0)
set(WAMR_BUILD_LIBC_WASI 1)
# mvp
set(WAMR_BUILD_BULK_MEMORY 1)
set(WAMR_BUILD_REF_TYPES 1)
set(WAMR_BUILD_SIMD 1)
set(WAMR_BUILD_TAIL_CALL 1)
set(WAMR_BUILD_THREAD_MGR 0)
set(WAMR_BUILD_LIB_WASI_THREADS 0)

## below two prevent valgrind from crashing
#TODO: there is a crash if turn it on
set(WAMR_DISABLE_HW_BOUND_CHECK 0)
set(WAMR_DISABLE_STACK_HW_BOUND_CHECK 1)
set(WAMR_DISABLE_WRITE_GS_BASE 1)

# vmlib
set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib SHARED ${WAMR_RUNTIME_LIB_SOURCE})
target_include_directories(vmlib INTERFACE ${WAMR_ROOT_DIR}/core/iwasm/include)
target_link_libraries(vmlib ${LLVM_AVAILABLE_LIBS} -lm -ldl)

################  samples  ################
add_subdirectory(import-memory)
add_subdirectory(import-table)

################ wasm and/or aot ################
include(ExternalProject)
ExternalProject_Add(wasm
  SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/wasm"
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/wasm -B build
                      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                      -DCMAKE_TOOLCHAIN_FILE=${WASISDK_TOOLCHAIN}
                      -DWAMR_BUILD_AOT=${WAMR_BUILD_AOT}
  BUILD_COMMAND     ${CMAKE_COMMAND} --build build
  INSTALL_COMMAND   ${CMAKE_COMMAND} --install build --prefix ${CMAKE_CURRENT_BINARY_DIR}
  EXCLUDE_FROM_ALL  NO
)
