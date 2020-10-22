# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (IWASM_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories (${IWASM_COMMON_DIR})

add_definitions(-DBH_MALLOC=wasm_runtime_malloc)
add_definitions(-DBH_FREE=wasm_runtime_free)

file (GLOB c_source_all ${IWASM_COMMON_DIR}/*.c)

if (WAMR_BUILD_TARGET STREQUAL "X86_64" OR WAMR_BUILD_TARGET STREQUAL "AMD_64")
  if (WAMR_BUILD_PLATFORM STREQUAL "windows")
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_em64.asm)
  else ()
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_em64.s)
  endif ()
elseif (WAMR_BUILD_TARGET STREQUAL "X86_32")
  if (WAMR_BUILD_PLATFORM STREQUAL "windows")
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_ia32.asm)
  else ()
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_ia32.s)
  endif ()
elseif (WAMR_BUILD_TARGET MATCHES "ARM.*")
  if (WAMR_BUILD_TARGET MATCHES "ARM.*_VFP")
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_arm_vfp.s)
  else ()
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_arm.s)
  endif ()
elseif (WAMR_BUILD_TARGET MATCHES "THUMB.*")
  if (WAMR_BUILD_TARGET MATCHES "THUMB.*_VFP")
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_thumb_vfp.s)
  else ()
    set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_thumb.s)
  endif ()
elseif (WAMR_BUILD_TARGET MATCHES "AARCH64.*")
  set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_aarch64.s)
elseif (WAMR_BUILD_TARGET STREQUAL "MIPS")
  set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_mips.s)
elseif (WAMR_BUILD_TARGET STREQUAL "XTENSA")
  set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_xtensa.s)
elseif (WAMR_BUILD_TARGET STREQUAL "GENERAL")
  # Use invokeNative_general.c instead of assembly code,
  # but the maximum number of native arguments is limited to 20,
  # and there are possible issues when passing arguments to
  # native function for some cpus, e.g. int64 and double arguments
  # in arm and mips need to be 8-bytes aligned, and some arguments
  # of x86_64 are passed by registers but not stack
  set (source_all ${c_source_all} ${IWASM_COMMON_DIR}/arch/invokeNative_general.c)
else ()
  message (FATAL_ERROR "Build target isn't set")
endif ()

set (IWASM_COMMON_SOURCE ${source_all})

