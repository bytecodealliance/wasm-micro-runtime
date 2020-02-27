# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (PLATFORM_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${PLATFORM_SHARED_DIR})
include_directories(${PLATFORM_SHARED_DIR}/../include)


file (GLOB_RECURSE source_all ${PLATFORM_SHARED_DIR}/*.c)

set (PLATFORM_SHARED_SOURCE ${source_all})

LIST (APPEND RUNTIME_LIB_HEADER_LIST "${PLATFORM_SHARED_DIR}/bh_platform.h")

file (GLOB header ${PLATFORM_SHARED_DIR}/../include/*.h)
LIST (APPEND RUNTIME_LIB_HEADER_LIST ${header})