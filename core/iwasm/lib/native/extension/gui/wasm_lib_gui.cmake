# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (WASM_LIB_GUI_DIR ${CMAKE_CURRENT_LIST_DIR})

set (THIRD_PARTY_DIR ${WASM_LIB_GUI_DIR}/../../../3rdparty)

include_directories(${WASM_LIB_GUI_DIR}
                    ${THIRD_PARTY_DIR}
                    ${THIRD_PARTY_DIR}/lvgl
                    ${THIRD_PARTY_DIR}/lvgl/src)

file (GLOB_RECURSE lvgl_source ${THIRD_PARTY_DIR}/lvgl/*.c)
file (GLOB_RECURSE wrapper_source ${WASM_LIB_GUI_DIR}/*.c)

set (WASM_LIB_GUI_SOURCE ${wrapper_source} ${lvgl_source})

