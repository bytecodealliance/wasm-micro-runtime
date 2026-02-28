# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-wasm-runtime)

# Enable test build flag
add_definitions(-DWAMR_BUILD_TEST=1)

# Test-specific feature configuration
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_APP_FRAMEWORK 0)

# Test both multi-module enabled and disabled
if(NOT DEFINED WAMR_BUILD_MULTI_MODULE)
  set(WAMR_BUILD_MULTI_MODULE 0)
endif()

include(../unit_common.cmake)

# Test source files
set(TEST_SOURCES
  test_runner.c
  mocks/wasm_loader_mock.c
  ${WAMR_RUNTIME_LIB_SOURCE}
)

# Create test executable
add_executable(wasm-runtime-test ${TEST_SOURCES})

target_include_directories(wasm-runtime-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/mocks
)

# Link dependencies
target_link_libraries(wasm-runtime-test cmocka::cmocka m)
target_link_options(wasm-runtime-test PRIVATE -Wl,--wrap=wasm_loader_load)

# Add to ctest
add_test(NAME wasm-runtime-test COMMAND wasm-runtime-test)
set_tests_properties(wasm-runtime-test PROPERTIES TIMEOUT 30)