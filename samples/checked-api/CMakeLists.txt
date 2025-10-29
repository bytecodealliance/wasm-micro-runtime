# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.14)

project(checked_api_sample)

# assertion required
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)
find_package(WASISDK REQUIRED)

################  runtime settings  ################
string (TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
include(CheckPIESupported)

# aot and interp by default
set(WAMR_BUILD_AOT 1)
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
# trap information
set(WAMR_BUILD_DUMP_CALL_STACK 1)

# vmlib
set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib SHARED ${WAMR_RUNTIME_LIB_SOURCE})
target_include_directories(vmlib INTERFACE ${WAMR_ROOT_DIR}/core/iwasm/include)
target_link_libraries (vmlib ${LLVM_AVAILABLE_LIBS} -lm -ldl)

################ host  ################
include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)
add_executable(${PROJECT_NAME} src/demo.c ${UNCOMMON_SHARED_SOURCE})
target_link_libraries(${PROJECT_NAME} vmlib)

################ aot + wasm  ################
include(ExternalProject)
ExternalProject_Add(wasm
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps"
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps -B build
                      -DCMAKE_TOOLCHAIN_FILE=${WASISDK_TOOLCHAIN}
  BUILD_COMMAND     ${CMAKE_COMMAND} --build build
  INSTALL_COMMAND   ${CMAKE_COMMAND} --install build --prefix ${CMAKE_CURRENT_BINARY_DIR}
)

enable_testing()
add_test(NAME checked_api_sample_test
  COMMAND ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
