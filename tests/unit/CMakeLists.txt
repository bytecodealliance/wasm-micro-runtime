# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 2.9)

project(unit-test)

SET(CMAKE_BUILD_TYPE Debug)

# add_definitions (-m32)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

if(WAMR_BUILD_TARGET STREQUAL "X86_32")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Fetch Google test
include (FetchContent)
FetchContent_Declare (
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable (googletest)

SET(GOOGLETEST_INCLUDED 1)

include(GoogleTest)
enable_testing()

add_subdirectory(wasm-vm)
add_subdirectory(interpreter)
add_subdirectory(aot)
add_subdirectory(wasm-c-api)
add_subdirectory(libc-builtin)
add_subdirectory(shared-utils)
add_subdirectory(running-modes)
add_subdirectory(runtime-common)
add_subdirectory(custom-section)
add_subdirectory(compilation)
add_subdirectory(linear-memory-wasm)
add_subdirectory(linear-memory-aot)
add_subdirectory(aot-stack-frame)
add_subdirectory(linux-perf)
add_subdirectory(gc)
add_subdirectory(memory64)
add_subdirectory(tid-allocator)
add_subdirectory(shared-heap)