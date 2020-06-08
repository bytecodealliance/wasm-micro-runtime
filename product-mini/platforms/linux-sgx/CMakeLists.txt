# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8)

project (iwasm)

set (WAMR_BUILD_PLATFORM "linux-sgx")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Set WAMR_BUILD_TARGET
if (NOT DEFINED WAMR_BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (WAMR_BUILD_TARGET "X86_64")
  else ()
    # Build as X86_32 by default in 32-bit platform
    set (WAMR_BUILD_TARGET "X86_32")
  endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

if (NOT DEFINED WAMR_BUILD_INTERP)
  # Enable Interpreter by default
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  # Disable AOT by default.
  # If enabling AOT, please install Intel SGX SDKv2.8 or later.
  set (WAMR_BUILD_AOT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_JIT)
  # Disable JIT by default.
  set (WAMR_BUILD_JIT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  # Enable libc builtin support by default
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_WASI)
  # Disable libc wasi support by default
  set (WAMR_BUILD_LIBC_WASI 0)
endif ()

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -ffunction-sections -fdata-sections \
                                     -Wall -Wno-unused-parameter -Wno-pedantic \
                                     -nostdinc -fvisibility=hidden -fpie" )

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set (SHARED_DIR ${WAMR_ROOT_DIR}/core/shared)
set (IWASM_DIR ${WAMR_ROOT_DIR}/core/iwasm)
set (APP_FRAMEWORK_DIR ${WAMR_ROOT_DIR}/core/app-framework)

# include the build config template file
include (${WAMR_ROOT_DIR}/build-scripts/config_common.cmake)

include_directories (${SHARED_DIR}/include
                     ${IWASM_DIR}/include)

enable_language (ASM)

include (${SHARED_DIR}/platform/${WAMR_BUILD_PLATFORM}/shared_platform.cmake)
include (${SHARED_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_DIR}/utils/shared_utils.cmake)
if (WAMR_BUILD_LIBC_BUILTIN EQUAL 1)
  include (${IWASM_DIR}/libraries/libc-builtin/libc_builtin.cmake)
endif ()
if (WAMR_BUILD_LIBC_WASI EQUAL 1)
  include (${IWASM_DIR}/libraries/libc-wasi/libc_wasi.cmake)
endif ()

include (${IWASM_DIR}/common/iwasm_common.cmake)

if (WAMR_BUILD_INTERP EQUAL 1 OR WAMR_BUILD_JIT EQUAL 1)
  include (${IWASM_DIR}/interpreter/iwasm_interp.cmake)
endif ()

if (WAMR_BUILD_AOT EQUAL 1)
  include (${IWASM_DIR}/aot/iwasm_aot.cmake)
  if (WAMR_BUILD_JIT EQUAL 1)
    include (${IWASM_DIR}/compilation/iwasm_compl.cmake)
  endif ()
endif ()

add_library (vmlib
             ${PLATFORM_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE}
             ${UTILS_SHARED_SOURCE}
             ${LIBC_BUILTIN_SOURCE}
             ${LIBC_WASI_SOURCE}
             ${IWASM_COMMON_SOURCE}
             ${IWASM_INTERP_SOURCE}
             ${IWASM_AOT_SOURCE}
             ${IWASM_COMPL_SOURCE})

