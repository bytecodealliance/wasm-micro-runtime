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

NAME := iwasm
IWASM_ROOT := iwasm
SHARED_LIB_ROOT := shared-lib

GLOBAL_DEFINES += NVALGRIND
GLOBAL_INCLUDES += ${IWASM_ROOT}/runtime/include \
                   ${IWASM_ROOT}/runtime/platform/include \
                   ${IWASM_ROOT}/runtime/platform/alios \
                   ${IWASM_ROOT}/runtime/vmcore-wasm \
                   ${SHARED_LIB_ROOT}/include \
                   ${SHARED_LIB_ROOT}/platform/include \
                   ${SHARED_LIB_ROOT}/platform/alios

$(NAME)_SOURCES := ${IWASM_ROOT}/runtime/utils/wasm_hashmap.c \
                   ${IWASM_ROOT}/runtime/utils/wasm_log.c \
                   ${IWASM_ROOT}/runtime/utils/wasm_dlfcn.c \
                   ${IWASM_ROOT}/runtime/platform/alios/wasm_native.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_application.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_interp.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_loader.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_runtime.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/invokeNative_general.c \
                   ${IWASM_ROOT}/lib/native/libc/libc_wrapper.c \
                   ${IWASM_ROOT}/lib/native/base/base_lib_export.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_platform.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_assert.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_thread.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_math.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/bh_memory.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_kfc.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_alloc.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_hmu.c \
                   src/main.c src/ext_lib_export.c

