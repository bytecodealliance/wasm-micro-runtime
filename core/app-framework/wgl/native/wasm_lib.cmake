# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (WASM_LIB_GUI_DIR ${CMAKE_CURRENT_LIST_DIR})

set (DEPS_DIR ${WASM_LIB_GUI_DIR}/../../../deps)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)

include_directories(${WASM_LIB_GUI_DIR}
                    ${DEPS_DIR}
                    ${DEPS_DIR}/lvgl
                    ${DEPS_DIR}/lvgl/src)

file (GLOB_RECURSE lvgl_source ${DEPS_DIR}/lvgl/*.c)
file (GLOB_RECURSE wrapper_source ${WASM_LIB_GUI_DIR}/*.c)

set (WASM_APP_LIB_CURRENT_SOURCE ${wrapper_source} ${lvgl_source})
