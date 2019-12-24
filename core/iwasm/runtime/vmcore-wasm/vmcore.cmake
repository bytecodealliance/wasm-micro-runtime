# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (VMCORE_LIB_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${VMCORE_LIB_DIR})
include_directories(${VMCORE_LIB_DIR}/../include)

file (GLOB_RECURSE c_source_all ${VMCORE_LIB_DIR}/*.c)
list (REMOVE_ITEM c_source_all ${VMCORE_LIB_DIR}/invokeNative_general.c)

if (${BUILD_TARGET} STREQUAL "X86_64" OR ${BUILD_TARGET} STREQUAL "AMD_64")
  set (source_all ${c_source_all} ${VMCORE_LIB_DIR}/invokeNative_em64.s)
elseif (${BUILD_TARGET} STREQUAL "X86_32")
  set (source_all ${c_source_all} ${VMCORE_LIB_DIR}/invokeNative_ia32.s)
elseif (${BUILD_TARGET} MATCHES "ARM.*")
  set (source_all ${c_source_all} ${VMCORE_LIB_DIR}/invokeNative_arm.s)
elseif (${BUILD_TARGET} MATCHES "THUMB.*")
  set (source_all ${c_source_all} ${VMCORE_LIB_DIR}/invokeNative_thumb.s)
elseif (${BUILD_TARGET} STREQUAL "MIPS")
  set (source_all ${c_source_all} ${VMCORE_LIB_DIR}/invokeNative_mips.s)
elseif (${BUILD_TARGET} STREQUAL "XTENSA")
  set (source_all ${c_source_all} ${VMCORE_LIB_DIR}/invokeNative_xtensa.s)
elseif (${BUILD_TARGET} STREQUAL "GENERAL")
  # Use invokeNative_general.c instead of assembly code,
  # but the maximum number of native arguments is limited to 20,
  # and there are possible issues when passing arguments to
  # native function for some cpus, e.g. int64 and double arguments
  # in arm and mips need to be 8-bytes aligned, and some arguments
  # of x86_64 are passed by registers but not stack
  set (source_all ${c_source_all} ${VMCORE_LIB_DIR}/invokeNative_general.c)
else ()
  message (FATAL_ERROR "Build target isn't set")
endif ()

set (VMCORE_LIB_SOURCE ${source_all})

