# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (test-custom-section)

add_definitions (-DRUN_ON_LINUX)

set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_LIBC_BUILTIN 0)
# to involve LLVM
set (WAMR_BUILD_JIT 1)
set (WAMR_BUILD_LAZY_JIT 0)
set (WAMR_BUILD_AOT 1)

add_definitions(-DWASM_ENABLE_WAMR_COMPILER=1)
add_definitions (-DWASM_ENABLE_DUMP_CALL_STACK=1)
add_definitions (-DWASM_ENABLE_AOT_STACK_FRAME=1)

# Feature to test
set (WAMR_BUILD_LOAD_CUSTOM_SECTION 1)

include (../unit_common.cmake)

include (${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

file (GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set (UNIT_SOURCE ${source_all})

set (unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${IWASM_COMPL_SOURCE}
)

# Automatically build wasm-apps for this test
add_subdirectory(wasm-apps)

# Now simply link against gtest or gtest_main as needed. Eg
add_executable (custom_section_test ${unit_test_sources})

target_link_libraries (custom_section_test ${LLVM_AVAILABLE_LIBS} gtest_main )

gtest_discover_tests(custom_section_test)
