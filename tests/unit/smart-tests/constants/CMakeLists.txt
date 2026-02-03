# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(enhanced_constants_test)

# Platform-specific definitions
add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_JIT 1)
set(WAMR_BUILD_FAST_JIT 0)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_MEMORY_PROFILING 0)

# Include shared unit test configuration
include(../../unit_common.cmake)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(i32_const_test_sources
        enhanced_i32_const_test.cc
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${IWASM_COMPL_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Prepare unit test sources for i64.const test
# set(i64_const_test_sources
#         enhanced_i64_const_test.cc
#         ${WAMR_RUNTIME_LIB_SOURCE}
#         ${IWASM_COMPL_SOURCE}
#         ${UNCOMMON_SHARED_SOURCE}
#         )

# Prepare unit test sources for f32.const test
set(f32_const_test_sources
        enhanced_f32_const_test.cc
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${IWASM_COMPL_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Prepare unit test sources for f64.const test
# set(f64_const_test_sources
#         enhanced_f64_const_test.cc
#         ${WAMR_RUNTIME_LIB_SOURCE}
#         ${IWASM_COMPL_SOURCE}
#         ${UNCOMMON_SHARED_SOURCE}
#         )

# Create test executables
add_executable(enhanced_i32_const_test ${i32_const_test_sources})
# add_executable(enhanced_i64_const_test ${i64_const_test_sources})
add_executable(enhanced_f32_const_test ${f32_const_test_sources})
# add_executable(enhanced_f64_const_test ${f64_const_test_sources})


# Link required libraries
target_link_libraries(enhanced_i32_const_test ${LLVM_AVAILABLE_LIBS} gtest_main)
# target_link_libraries(enhanced_i64_const_test ${LLVM_AVAILABLE_LIBS} gtest_main)
target_link_libraries(enhanced_f32_const_test ${LLVM_AVAILABLE_LIBS} gtest_main)
# target_link_libraries(enhanced_f64_const_test ${LLVM_AVAILABLE_LIBS} gtest_main)

# Post-build: Copy WASM test files to build directory for all tests
add_custom_command(TARGET enhanced_i32_const_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
        $<TARGET_FILE_DIR:enhanced_i32_const_test>/wasm-apps
    COMMENT "Copying WASM test files to i32_const test build directory"
)

# add_custom_command(TARGET enhanced_i64_const_test POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
#         $<TARGET_FILE_DIR:enhanced_i64_const_test>/wasm-apps
#     COMMENT "Copying WASM test files to i64_const test build directory"
# )

add_custom_command(TARGET enhanced_f32_const_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
        $<TARGET_FILE_DIR:enhanced_f32_const_test>/wasm-apps
    COMMENT "Copying WASM test files to f32_const test build directory"
)

# add_custom_command(TARGET enhanced_f64_const_test POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
#         $<TARGET_FILE_DIR:enhanced_f64_const_test>/wasm-apps
#     COMMENT "Copying WASM test files to f64_const test build directory"
# )

include(GoogleTest)

gtest_discover_tests(enhanced_i32_const_test
                     PROPERTIES RUN_SERIAL TRUE
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# gtest_discover_tests(enhanced_i64_const_test
#                      PROPERTIES RUN_SERIAL TRUE
#                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(enhanced_f32_const_test
                     PROPERTIES RUN_SERIAL TRUE
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# gtest_discover_tests(enhanced_f64_const_test
#                      PROPERTIES RUN_SERIAL TRUE
#                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
