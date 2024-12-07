# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (LIBC_BUILTIN_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_LIBC_BUILTIN=1)

include_directories (${LIBC_BUILTIN_DIR})

list (APPEND source_all ${LIBC_BUILTIN_DIR}/libc_builtin_wrapper.c)

if (WAMR_BUILD_SPEC_TEST EQUAL 1)
  list (APPEND source_all ${LIBC_BUILTIN_DIR}/spec_test_builtin_wrapper.c)
endif ()

if (WAMR_BUILD_WASI_TEST EQUAL 1)
  list (APPEND source_all ${LIBC_BUILTIN_DIR}/wasi_test_builtin_wrapper.c)
endif ()

set (LIBC_BUILTIN_SOURCE ${source_all})
