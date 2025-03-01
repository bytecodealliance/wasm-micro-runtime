# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(wasm_fuzzing LANGUAGES C CXX)

# Ensure Clang is used as the compiler
if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "Please use Clang as the C compiler for libFuzzer compatibility.")
endif()

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)

# Reset default linker flags
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Determine WAMR_BUILD_TARGET based on system properties
if(NOT DEFINED WAMR_BUILD_TARGET)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
    set(WAMR_BUILD_TARGET "AARCH64")
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    set(WAMR_BUILD_TARGET "RISCV64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WAMR_BUILD_TARGET "X86_64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(WAMR_BUILD_TARGET "X86_32")
  else()
    message(SEND_ERROR "Unsupported build target platform!")
  endif()
endif()

if(APPLE)
  add_definitions(-DBH_PLATFORM_DARWIN)
endif()

# Disable hardware bound check and enable AOT validator
set(WAMR_DISABLE_HW_BOUND_CHECK 1)
set(WAMR_BUILD_AOT_VALIDATOR 1)

set(REPO_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
message(STATUS "REPO_ROOT_DIR: ${REPO_ROOT_DIR}")

set(LLVM_SRC_ROOT ${REPO_ROOT_DIR}/core/deps/llvm)
set(LLVM_BUILD_ROOT ${LLVM_SRC_ROOT}/build)
set(LLVM_DIR ${LLVM_BUILD_ROOT}/lib/cmake/llvm)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Enable fuzzer
add_definitions(-DWASM_ENABLE_FUZZ_TEST=1)
add_compile_options(-fsanitize=fuzzer)
add_link_options(-fsanitize=fuzzer)

# Enable sanitizers if not in oss-fuzz environment
set(CFLAGS_ENV $ENV{CFLAGS})
string(FIND "${CFLAGS_ENV}" "-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION" IN_OSS_FUZZ)
if(IN_OSS_FUZZ EQUAL -1)
  message(STATUS "Enable ASan and UBSan in non-oss-fuzz environment")
  add_compile_options(
    -fprofile-instr-generate -fcoverage-mapping
    -fno-sanitize-recover=all
    -fsanitize=address,undefined
    -fsanitize=float-divide-by-zero,unsigned-integer-overflow,local-bounds,nullability
    -fno-sanitize=alignment
  )
  add_link_options(-fsanitize=address -fprofile-instr-generate)
endif()

add_subdirectory(wasm_mutator)
add_subdirectory(aot_compiler)
