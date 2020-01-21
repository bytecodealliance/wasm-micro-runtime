# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (WASM_APP_GUI_DIR ${CMAKE_CURRENT_LIST_DIR})

set (DEPS_DIR ${WASM_APP_GUI_DIR}/../../../deps)

include_directories(${WASM_APP_GUI_DIR}
                    ${DEPS_DIR}
                    ${DEPS_DIR}/lvgl
                    ${DEPS_DIR}/lvgl/src)

file (GLOB_RECURSE source_all ${WASM_APP_GUI_DIR}/src/*.c)

set (WASM_APP_CURRENT_SOURCE ${source_all})
