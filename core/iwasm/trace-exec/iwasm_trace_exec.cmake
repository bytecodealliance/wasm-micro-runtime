# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_definitions (-DWASM_ENABLE_TRACE_MODE=1)

set(IWASM_TRACE_EXEC_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${IWASM_TRACE_EXEC_DIR})

set(IWASM_TRACE_EXEC_SOURCE
  ${IWASM_TRACE_EXEC_DIR}/trace_exec.c
)
