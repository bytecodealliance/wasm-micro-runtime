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

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language (ASM)

add_definitions (-DNVALGRIND)

set (IWASM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/iwasm)
set (SHARED_LIB_ROOT ${IWASM_ROOT}/../shared-lib)

include_directories (${IWASM_ROOT}/runtime/include
                     ${IWASM_ROOT}/runtime/platform/include
                     ${IWASM_ROOT}/runtime/platform/zephyr
                     ${IWASM_ROOT}/runtime/vmcore_wasm
                     ${SHARED_LIB_ROOT}/include
                     ${SHARED_LIB_ROOT}/platform/include
                     ${SHARED_LIB_ROOT}/platform/zephyr)

set (IWASM_SRCS ${IWASM_ROOT}/runtime/utils/wasm_hashmap.c
                ${IWASM_ROOT}/runtime/utils/wasm_log.c
                ${IWASM_ROOT}/runtime/utils/wasm_dlfcn.c
                ${IWASM_ROOT}/runtime/platform/zephyr/wasm_math.c
                ${IWASM_ROOT}/runtime/platform/zephyr/wasm_platform.c
                ${IWASM_ROOT}/runtime/platform/zephyr/wasm-native.c
                ${IWASM_ROOT}/runtime/vmcore_wasm/wasm-application.c
                ${IWASM_ROOT}/runtime/vmcore_wasm/wasm-interp.c
                ${IWASM_ROOT}/runtime/vmcore_wasm/wasm-loader.c
                ${IWASM_ROOT}/runtime/vmcore_wasm/wasm-runtime.c
                ${IWASM_ROOT}/runtime/vmcore_wasm/invokeNative_general.c
                ${IWASM_ROOT}/lib/native/libc/libc_wrapper.c
                ${IWASM_ROOT}/lib/native/base/base-lib-export.c
                ${SHARED_LIB_ROOT}/platform/zephyr/bh_platform.c
                ${SHARED_LIB_ROOT}/platform/zephyr/bh_assert.c
                ${SHARED_LIB_ROOT}/platform/zephyr/bh_thread.c
                ${SHARED_LIB_ROOT}/mem-alloc/bh_memory.c
                ${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.c
                ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_kfc.c
                ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_alloc.c
                ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_hmu.c)

target_sources(app PRIVATE ${IWASM_SRCS} src/main.c src/ext-lib-export.c)
