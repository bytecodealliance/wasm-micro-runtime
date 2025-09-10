# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.14)
project (wasm_c_api_test)

set(WAMR_BUILD_PLATFORM "linux")

# WAMR features switch
if (NOT DEFINED WAMR_BUILD_TARGET)
  set(WAMR_BUILD_TARGET "X86_64")
endif()
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_FAST_INTERP 0)

include (../unit_common.cmake)

add_executable(wasm_c_api_test basic.cc ${WAMR_RUNTIME_LIB_SOURCE})
target_link_libraries(wasm_c_api_test gtest_main)
gtest_discover_tests(wasm_c_api_test)
