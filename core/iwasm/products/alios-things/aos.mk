# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

NAME := iwasm
IWASM_ROOT := iwasm
SHARED_LIB_ROOT := shared-lib

# Change it to THUMBV7M if you want to build for developerkit
BUILD_TARGET := X86_32

ifeq (${BUILD_TARGET}, X86_32)
  GLOBAL_DEFINES += BUILD_TARGET_X86_32
  INVOKE_NATIVE := invokeNative_ia32.s
else ifeq (${BUILD_TARGET}, X86_64)
  GLOBAL_DEFINES += BUILD_TARGET_X86_64
  INVOKE_NATIVE := invokeNative_em64.s
else ifeq ($(findstring ARM,$(BUILD_TARGET)), ARM)
  GLOBAL_DEFINES += BUILD_TARGET_ARM
  GLOBAL_DEFINES += BUILD_TARGET=${BUILD_TARGET}
  INVOKE_NATIVE := invokeNative_arm.s
else ifeq ($(findstring THUMB,$(BUILD_TARGET)), THUMB)
  GLOBAL_DEFINES += BUILD_TARGET_THUMB
  GLOBAL_DEFINES += BUILD_TARGET=${BUILD_TARGET}
  INVOKE_NATIVE := invokeNative_thumb.s
else ifeq (${BUILD_TARGET}, MIPS)
  GLOBAL_DEFINES += BUILD_TARGET_MIPS
  INVOKE_NATIVE := invokeNative_mips.s
else ifeq (${BUILD_TARGET}, XTENSA)
  GLOBAL_DEFINES += BUILD_TARGET_XTENSA
  INVOKE_NATIVE := invokeNative_xtensa.s
else
  $(error Build target isn't set)
endif

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
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_application.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_interp.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_loader.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_runtime.c \
                   ${IWASM_ROOT}/runtime/vmcore-wasm/${INVOKE_NATIVE} \
                   ${IWASM_ROOT}/lib/native/libc/libc_builtin_wrapper.c \
                   ${IWASM_ROOT}/lib/native/base/base_lib_export.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_platform.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_definition.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_assert.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_thread.c \
                   ${SHARED_LIB_ROOT}/platform/alios/bh_math.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/bh_memory.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_kfc.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_alloc.c \
                   ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_hmu.c \
                   src/main.c src/ext_lib_export.c

