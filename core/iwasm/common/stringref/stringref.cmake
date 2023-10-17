# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (IWASM_STRINGREF_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_STRINGREF=1)

include_directories (${IWASM_STRINGREF_DIR})

file (GLOB_RECURSE source_all ${IWASM_STRINGREF_DIR}/*.c)

if (NOT DEFINED WAMR_STRINGREF_IMPL_SOURCE)
    set (IWASM_STRINGREF_SOURCE ${source_all})
else ()
    set (IWASM_STRINGREF_SOURCE ${WAMR_STRINGREF_IMPL_SOURCE})
endif ()
