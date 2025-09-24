# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (test-wasm-vm)

add_definitions (-DRUN_ON_LINUX)

set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)

add_definitions (-Dattr_container_malloc=malloc)
add_definitions (-Dattr_container_free=free)

include (../unit_common.cmake)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

file (GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set (UNIT_SOURCE ${source_all})

set (unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
)

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(wasm_vm_test ${unit_test_sources})

target_link_libraries(wasm_vm_test ${LLVM_AVAILABLE_LIBS} gtest_main)

add_custom_command(TARGET wasm_vm_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/app4/*.wasm
                ${PROJECT_BINARY_DIR}/
    COMMENT "Copy wasm files to the output directory"
)

gtest_discover_tests(wasm_vm_test)
