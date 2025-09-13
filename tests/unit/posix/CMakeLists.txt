# Copyright (C) 2025 WAMR Community.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-posix-platform)

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 1)

include(../unit_common.cmake)

# Include paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Step 6: posix_clock_test.cc, posix_sleep_test.cc, posix_time_test.cc, posix_malloc_test.cc, posix_file_test.cc, posix_socket_test.cc, and posix_blocking_op_test.cc
set(POSIX_TEST_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_clock_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_sleep_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_time_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_malloc_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_file_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_socket_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_blocking_op_test.cc
)

# POSIX platform sources - Step 6: posix_clock.c, posix_sleep.c, posix_time.c, posix_malloc.c, posix_file.c, posix_socket.c, and posix_blocking_op.c
set(POSIX_PLATFORM_DIR ${WAMR_ROOT_DIR}/core/shared/platform/common/posix)
set(POSIX_SOURCE_FILES
    ${POSIX_PLATFORM_DIR}/posix_clock.c
    ${POSIX_PLATFORM_DIR}/posix_sleep.c
    ${POSIX_PLATFORM_DIR}/posix_time.c
    ${POSIX_PLATFORM_DIR}/posix_malloc.c
    ${POSIX_PLATFORM_DIR}/posix_file.c
    ${POSIX_PLATFORM_DIR}/posix_socket.c
    ${POSIX_PLATFORM_DIR}/posix_blocking_op.c
)

# All sources for the test executable
set(unit_test_sources
    ${POSIX_TEST_SOURCE}
    ${POSIX_SOURCE_FILES}
    ${WAMR_RUNTIME_LIB_SOURCE}
)

# Create test executable
add_executable(posix_platform_test ${unit_test_sources})
enable_testing()
include(GoogleTest)
# Link with Google Test
target_link_libraries(posix_platform_test gtest_main)

# Discover tests
gtest_discover_tests(posix_platform_test)