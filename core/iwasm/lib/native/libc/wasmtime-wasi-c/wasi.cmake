# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (WASI_LIB_DIR ${CMAKE_CURRENT_LIST_DIR})

if (WASM_ENABLE_WASI EQUAL 1)
  include_directories(${WASI_LIB_DIR}/sandboxed-system-primitives/include
                      ${WASI_LIB_DIR}/sandboxed-system-primitives/src
                     )
  file (GLOB_RECURSE source_all ${WASI_LIB_DIR}/sandboxed-system-primitives/*.c )
  set (WASI_LIB_SOURCE ${source_all})
endif ()
