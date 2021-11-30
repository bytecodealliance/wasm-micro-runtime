#
# Copyright (C) 2020 Quux Oy
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

add_definitions (-DBH_PLATFORM_QNX7=)  # Note: definition must not be empty, or QNX asm choces on it

set (PLATFORM_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories (${PLATFORM_SHARED_DIR})
include_directories (${PLATFORM_SHARED_DIR}/../include)

include (${CMAKE_CURRENT_LIST_DIR}/../common/posix/platform_api_posix.cmake)

file (GLOB_RECURSE source_all ${PLATFORM_SHARED_DIR}/*.c)

set (PLATFORM_SHARED_SOURCE ${source_all} ${PLATFORM_COMMON_POSIX_SOURCE})

file (GLOB header ${PLATFORM_SHARED_DIR}/../include/*.h)
LIST (APPEND RUNTIME_LIB_HEADER_LIST ${header})
