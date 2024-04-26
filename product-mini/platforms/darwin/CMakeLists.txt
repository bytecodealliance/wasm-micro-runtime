# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.9)

project (iwasm)

set (WAMR_BUILD_PLATFORM "darwin")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

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

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(WAMR_BUILD_LIB_PTHREAD 1)
set(WAMR_BUILD_LIB_PTHREAD_SEMAPHORE 1)
set(WAMR_BUILD_LIB_WASI_THREADS 1)
set(WAMR_BUILD_THREAD_MGR 1)
set(WAMR_BUILD_PLATFORM "darwin")

set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_DEBUG_INTERP 0)
set(WAMR_BUILD_DEBUG_AOT 0)
set(WAMR_BUILD_FAST_JIT 1)
set(WAMR_BUILD_WASI_NN 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_DUMP_CALL_STACK 1)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_SHARED_MEMORY 1)
set(WAMR_BUILD_BULK_MEMORY 1)
set(WAMR_BUILD_REF_TYPES 0)
set(WAMR_BUILD_SIMD 1)
set(WAMR_BUILD_CUSTOM_NAME_SECTION 1)
set(WAMR_BUILD_CHECKPOINT_RESTORE 1)
set(WAMR_DISABLE_HW_BOUND_CHECK 1)

set (CMAKE_SHARED_LINKER_FLAGS "-Wl,-U,_get_ext_lib_export_apis")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

set (CMAKE_MACOSX_RPATH True)

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})

include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

add_executable (iwasm main.c ${UNCOMMON_SHARED_SOURCE})

install (TARGETS iwasm DESTINATION bin)

target_link_libraries (iwasm vmlib ${LLVM_AVAILABLE_LIBS} ${UV_A_LIBS} -lm -ldl -lpthread)

add_library (libiwasm SHARED ${WAMR_RUNTIME_LIB_SOURCE})

install (TARGETS libiwasm DESTINATION lib)

set_target_properties (libiwasm PROPERTIES OUTPUT_NAME iwasm)

target_link_libraries (libiwasm ${LLVM_AVAILABLE_LIBS} ${UV_A_LIBS} -lm -ldl -lpthread)

