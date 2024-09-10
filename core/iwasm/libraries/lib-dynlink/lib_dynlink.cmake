# Copyright (C) 2024 Amazon.com Inc. or its affiliates. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (LIB_DYNLINK_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_DYNAMIC_LINKING=1)

include_directories(${LIB_DYNLINK_DIR})

set (LIB_DYNLINK_SOURCE
    ${LIB_DYNLINK_DIR}/dynlink_section_loader.c
    ${LIB_DYNLINK_DIR}/dynlink_types.c)
