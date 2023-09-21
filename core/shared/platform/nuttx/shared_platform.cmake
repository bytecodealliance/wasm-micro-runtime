# Copyright (C) 2020 XiaoMi Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (PLATFORM_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions(-DBH_PLATFORM_NUTTX)

include_directories(${PLATFORM_SHARED_DIR})
include_directories(${PLATFORM_SHARED_DIR}/../include)

file (GLOB_RECURSE source_all ${PLATFORM_SHARED_DIR}/*.c)

if (WAMR_BUILD_LIBC_WASI EQUAL 1)
  list(APPEND source_all ${PLATFORM_SHARED_DIR}/../common/posix/posix_file.c)
endif ()

set (PLATFORM_SHARED_SOURCE ${source_all} ${PLATFORM_COMMON_MATH_SOURCE})

