# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 
set (PLATFORM_COMMON_MATH_DIR ${CMAKE_CURRENT_LIST_DIR})

file (GLOB_RECURSE math_source_all ${PLATFORM_COMMON_MATH_DIR}/*.c)

set (PLATFORM_COMMON_MATH_SOURCE ${math_source_all} )
