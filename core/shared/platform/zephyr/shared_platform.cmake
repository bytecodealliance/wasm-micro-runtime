# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (PLATFORM_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions(-DBH_PLATFORM_ZEPHYR)

include_directories(${PLATFORM_SHARED_DIR})
include_directories(${PLATFORM_SHARED_DIR}/../include)

include (${CMAKE_CURRENT_LIST_DIR}/../common/math/platform_api_math.cmake)

file (GLOB_RECURSE source_all ${PLATFORM_SHARED_DIR}/*.c)

if (WAMR_BUILD_LIBC_WASI EQUAL 1)
  list(APPEND source_all ${PLATFORM_SHARED_DIR}/../common/posix/posix_file.c)
  include (${CMAKE_CURRENT_LIST_DIR}/../common/libc-util/platform_common_libc_util.cmake)
  set(source_all ${source_all} ${PLATFORM_COMMON_LIBC_UTIL_SOURCE})
endif ()

set (PLATFORM_SHARED_SOURCE ${source_all} ${PLATFORM_COMMON_MATH_SOURCE})

