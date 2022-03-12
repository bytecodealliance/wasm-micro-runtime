# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (IWASM_FAST_JIT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_FAST_JIT=1)

include_directories (${IWASM_FAST_JIT_DIR})

if (WAMR_BUILD_TARGET STREQUAL "X86_64" OR WAMR_BUILD_TARGET STREQUAL "AMD_64")
    include(FetchContent)
    FetchContent_Declare(
        asmjit
        GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    )
    FetchContent_GetProperties(asmjit)
    if(NOT asmjit_POPULATED)
        message ("-- Fetching asmjit ..")
        FetchContent_Populate(asmjit)
        add_definitions(-DASMJIT_STATIC)
        add_definitions(-DASMJIT_NO_DEPRECATED)
        add_definitions(-DASMJIT_NO_BUILDER)
        add_definitions(-DASMJIT_NO_COMPILER)
        add_definitions(-DASMJIT_NO_JIT)
        add_definitions(-DASMJIT_NO_LOGGING)
        add_definitions(-DASMJIT_NO_TEXT)
        add_definitions(-DASMJIT_NO_VALIDATION)
        add_definitions(-DASMJIT_NO_INTROSPECTION)
        add_definitions(-DASMJIT_NO_INTRINSICS)
        add_definitions(-DASMJIT_NO_AARCH64)
        add_definitions(-DASMJIT_NO_AARCH32)
        include_directories("${asmjit_SOURCE_DIR}/src")
        add_subdirectory(${asmjit_SOURCE_DIR} ${asmjit_BINARY_DIR} EXCLUDE_FROM_ALL)
        file (GLOB_RECURSE cpp_source_asmjit
            ${asmjit_SOURCE_DIR}/src/asmjit/core/*.cpp
            ${asmjit_SOURCE_DIR}/src/asmjit/x86/*.cpp
        )
    endif()
endif()

file (GLOB c_source_jit ${IWASM_FAST_JIT_DIR}/*.c ${IWASM_FAST_JIT_DIR}/fe/*.c)

if (WAMR_BUILD_TARGET STREQUAL "X86_64" OR WAMR_BUILD_TARGET STREQUAL "AMD_64")
  file (GLOB_RECURSE cpp_source_jit_cg ${IWASM_FAST_JIT_DIR}/cg/x86-64/*.cpp)
else ()
  message (FATAL_ERROR "Fast JIT codegen for target ${WAMR_BUILD_TARGET} isn't implemented")
endif ()

set (IWASM_FAST_JIT_SOURCE ${c_source_jit} ${cpp_source_jit_cg} ${cpp_source_asmjit})
