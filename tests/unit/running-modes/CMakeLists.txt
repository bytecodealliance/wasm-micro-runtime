# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-running-modes)

# Compile wasm modules
add_subdirectory(wasm-apps)

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_JIT 1)
set(WAMR_BUILD_FAST_JIT 1)

# if only load this CMake other than load it as subdirectory
include(../unit_common.cmake)

include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

set(unit_test_sources
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
  ${SRC_LIST}
)

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(wasm_running_modes_test ${unit_test_sources})

target_link_libraries(wasm_running_modes_test ${LLVM_AVAILABLE_LIBS} gtest_main)

gtest_discover_tests(wasm_running_modes_test)
