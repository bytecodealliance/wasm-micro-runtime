# Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-shared-heap)

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_JIT 0)
if(WAMR_BUILD_TARGET STREQUAL "X86_32")
    set(WAMR_BUILD_MEMORY64 0)
else()
    set(WAMR_BUILD_MEMORY64 1)
endif()
set(WAMR_BUILD_SHARED_HEAP 1)

# Compile wasm modules
add_subdirectory(wasm-apps)

if (WAMR_BUILD_MEMORY64 EQUAL 1)
   add_subdirectory(wasm-apps/memory64)
endif ()

# if only load this CMake other than load it as subdirectory
include(../unit_common.cmake)

include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(UNIT_SOURCE ${source_all})

set(unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
)

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(shared_heap_test ${unit_test_sources})

target_link_libraries(shared_heap_test gtest_main)

gtest_discover_tests(shared_heap_test)
