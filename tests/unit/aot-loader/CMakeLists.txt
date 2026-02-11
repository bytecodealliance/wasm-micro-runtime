# Copyright (C) 2024 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-aot-loader)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
SET(CMAKE_BUILD_TYPE Debug)

add_definitions(-Dattr_container_malloc=malloc)
add_definitions(-Dattr_container_free=free)

if (APPLE)
  set(WAMR_BUILD_PLATFORM "darwin")
  if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(WAMR_BUILD_TARGET "AARCH64")
  endif()
endif()

set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_APP_FRAMEWORK 0)

# Skip LLVM discovery - we only need the runtime, not the compiler
set(LLVM_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE PATH "Dummy LLVM dir")

include(../unit_common.cmake)

# Fetch Google Test
include(FetchContent)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
  )
else()
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
endif()
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)
enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${IWASM_DIR}/aot)

file(GLOB source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(UNIT_SOURCE ${source_all})

set(unit_test_sources
    ${UNIT_SOURCE}
    ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(aot_loader_test ${unit_test_sources})

target_link_libraries(aot_loader_test gtest_main)

gtest_discover_tests(aot_loader_test)
