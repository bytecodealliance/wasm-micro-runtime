# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception


set (MEM_ALLOC_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${MEM_ALLOC_DIR})

if (NOT ENABLE_SNMALLOC)

      file (GLOB_RECURSE source_all
            ${MEM_ALLOC_DIR}/ems/*.c
            ${MEM_ALLOC_DIR}/tlsf/*.c
            ${MEM_ALLOC_DIR}/mem_alloc.c)

else()

      file (GLOB_RECURSE source_all
            ${MEM_ALLOC_DIR}/mem_alloc_snmalloc.cc)

endif()

set (MEM_ALLOC_SHARED_SOURCE ${source_all})

