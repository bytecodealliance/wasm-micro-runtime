#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

set (WAMR_DIR ${CMAKE_CURRENT_LIST_DIR}/deps/wamr-gc)

set (WAMR_BUILD_PLATFORM "linux")

set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_LIBC_BUILTIN 1)
set (WAMR_BUILD_GC_BINARYEN 1)

if (NOT DEFINED WAMR_BUILD_TARGET)
    set (WAMR_BUILD_TARGET X86_64)
endif()

include(${WAMR_DIR}/build-scripts/runtime_lib.cmake)
