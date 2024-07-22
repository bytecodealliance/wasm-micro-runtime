# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8)

if (NOT DEFINED CMAKE_C_COMPILER)
set (CMAKE_C_COMPILER "clang")
endif ()
if (NOT DEFINED CMAKE_CXX_COMPILER)
set (CMAKE_CXX_COMPILER "clang++")
endif ()

project(wasm_mutator)

set (CMAKE_BUILD_TYPE Debug)

string (TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 17)

# Set WAMR_BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "AARCH64[sub]", "ARM[sub]", "THUMB[sub]",
# "MIPS", "XTENSA", "RISCV64[sub]", "RISCV32[sub]"
if (NOT DEFINED WAMR_BUILD_TARGET)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
    set (WAMR_BUILD_TARGET "AARCH64")
  elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    set (WAMR_BUILD_TARGET "RISCV64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (WAMR_BUILD_TARGET "X86_64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Build as X86_32 by default in 32-bit platform
    set (WAMR_BUILD_TARGET "X86_32")
  else ()
    message(SEND_ERROR "Unsupported build target platform!")
  endif ()
endif ()

if (APPLE)
  add_definitions(-DBH_PLATFORM_DARWIN)
endif ()

if(CUSTOM_MUTATOR EQUAL 1)
  add_compile_definitions(CUSTOM_MUTATOR)
endif()

if (NOT DEFINED WAMR_BUILD_INTERP)
  # Enable Interpreter by default
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  # Enable AOT by default.
  set (WAMR_BUILD_AOT 1)
endif ()

if (NOT DEFINED WAMR_BUILD_JIT)
  # Disable JIT by default.
  set (WAMR_BUILD_JIT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  # Enable libc builtin support by default
  set (WAMR_BUILD_LIBC_BUILTIN 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_WASI)
  # Enable libc wasi support by default
  set (WAMR_BUILD_LIBC_WASI 0)
endif ()

if (NOT DEFINED WAMR_BUILD_FAST_INTERP)
  # Enable fast interpreter
  set (WAMR_BUILD_FAST_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_MULTI_MODULE)
  # Enable multiple modules
  set (WAMR_BUILD_MULTI_MODULE 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIB_PTHREAD)
  # Disable pthread library by default
  set (WAMR_BUILD_LIB_PTHREAD 0)
endif ()

if (NOT DEFINED WAMR_BUILD_MINI_LOADER)
  # Disable wasm mini loader by default
  set (WAMR_BUILD_MINI_LOADER 0)
endif ()

if (NOT DEFINED WAMR_BUILD_SIMD)
  # Enable SIMD by default
  set (WAMR_BUILD_SIMD 1)
endif ()

if (NOT DEFINED WAMR_BUILD_REF_TYPES)
  # Enable reference type by default
  set (WAMR_BUILD_REF_TYPES 1)
endif ()

if (NOT DEFINED WAMR_BUILD_DEBUG_INTERP)
  # Disable Debug feature by default
  set (WAMR_BUILD_DEBUG_INTERP 0)
endif ()

if (WAMR_BUILD_DEBUG_INTERP EQUAL 1)
  set (WAMR_BUILD_FAST_INTERP 0)
  set (WAMR_BUILD_MINI_LOADER 0)
  set (WAMR_BUILD_SIMD 0)
endif ()

set (REPO_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
message([ceith]:REPO_ROOT_DIR, ${REPO_ROOT_DIR})

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_definitions(-DWAMR_USE_MEM_POOL=0 -DWASM_ENABLE_FUZZ_TEST=1)

# Enable fuzzer
add_compile_options(-fsanitize=fuzzer)
add_link_options(-fsanitize=fuzzer)

# if not calling from oss-fuzz helper, enable all support sanitizers
# oss-fuzz will define FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION in CFLAGS and CXXFLAGS
set(CFLAGS_ENV $ENV{CFLAGS})
string(FIND "${CFLAGS_ENV}" "-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION" IN_OSS_FUZZ)
if (IN_OSS_FUZZ EQUAL -1)
  message("[ceith]:Enable ASan and UBSan in non-oss-fuzz environment")
  add_compile_options(
    -fprofile-instr-generate -fcoverage-mapping
    -fno-sanitize-recover=all
    -fsanitize=address,undefined
    # reference: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
    # -fsanitize=undefined: All of the checks listed above other than float-divide-by-zero,
    #     unsigned-integer-overflow, implicit-conversion, local-bounds and
    #     the nullability-* group of checks.
    #
    # for now, we disable below from UBSan
    # -alignment
    # -implicit-conversion
    #
    -fsanitize=float-divide-by-zero,unsigned-integer-overflow,local-bounds,nullability
    -fno-sanitize=alignment
  )
  add_link_options(-fsanitize=address -fprofile-instr-generate)
endif ()

include(${REPO_ROOT_DIR}/core/shared/utils/uncommon/shared_uncommon.cmake)
include(${REPO_ROOT_DIR}/build-scripts/runtime_lib.cmake)

add_library(vmlib
    ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(wasm_mutator_fuzz wasm_mutator_fuzz.cc)
target_link_libraries(wasm_mutator_fuzz vmlib -lm)
