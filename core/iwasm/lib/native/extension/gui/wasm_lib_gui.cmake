# Copyright (C) 2019 Intel Corporation.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set (WASM_LIB_GUI_DIR ${CMAKE_CURRENT_LIST_DIR})

set (THIRD_PARTY_DIR ${WASM_LIB_GUI_DIR}/../../../3rdparty)

include_directories(${WASM_LIB_GUI_DIR} ${THIRD_PARTY_DIR} ${THIRD_PARTY_DIR}/lvgl)

file (GLOB_RECURSE lvgl_source ${THIRD_PARTY_DIR}/lvgl/*.c)
file (GLOB_RECURSE wrapper_source ${WASM_LIB_GUI_DIR}/*.c)

set (WASM_LIB_GUI_SOURCE ${wrapper_source} ${lvgl_source})

