# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.9)
project (wasm_c_api_test)

################  runtime settings  ################
set(CMAKE_BUILD_TYPE Debug)
set(WAMR_BUILD_PLATFORM "linux")

# Resetdefault linker flags
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# WAMR features switch
set(WAMR_BUILD_TARGET "X86_64")
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_FAST_INTERP 0)

# compiling and linking flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -pie -fPIE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-security -mindirect-branch-register")

# build out vmlib
# hard code path here
set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

add_library(vmlib STATIC ${WAMR_RUNTIME_LIB_SOURCE})
################################################

################  unit test related  ################
# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.

if (NOT (GOOGLETEST_INCLUDED EQUAL 1))
# Prevent overriding the parent project's compiler/linker
# settings on Windows
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Fetch Google test
include (FetchContent)
FetchContent_Declare (
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable (googletest)
endif()

enable_testing()

add_executable(wasm_c_api_test
  basic.cc
)

target_link_libraries(wasm_c_api_test vmlib gtest_main)

gtest_discover_tests(wasm_c_api_test)
