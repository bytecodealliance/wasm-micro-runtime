# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

CORE_ROOT := wamr/core
IWASM_ROOT := wamr/core/iwasm
SHARED_ROOT := wamr/core/shared

ifeq ($(CONFIG_ARCH_ARMV7M),y)
WAMR_BUILD_TARGET := THUMBV7EM
else ifeq ($(CONFIG_ARCH_ARMV8M),y)
WAMR_BUILD_TARGET := THUMBV8M
else ifeq ($(CONFIG_ARCH_X86),y)
WAMR_BUILD_TARGET := X86_32
else ifeq ($(CONFIG_ARCH_X86_64),y)
WAMR_BUILD_TARGET := X86_64
else ifeq ($(CONFIG_ARCH_XTENSA),y)
WAMR_BUILD_TARGET := XTENSA
endif

WAMR_BUILD_PLATFORM := nuttx

ifeq (${WAMR_BUILD_TARGET}, X86_32)
  CFLAGS += -DBUILD_TARGET_X86_32
  INVOKE_NATIVE := invokeNative_ia32.s
  AOT_RELOC := aot_reloc_x86_32.c
else ifeq (${WAMR_BUILD_TARGET}, X86_64)
  CFLAGS += -DBUILD_TARGET_X86_64
  INVOKE_NATIVE := invokeNative_em64.s
  AOT_RELOC := aot_reloc_x86_64.c
else ifeq ($(findstring ARM,$(WAMR_BUILD_TARGET)), ARM)
  CFLAGS += -DBUILD_TARGET_ARM
  CFLAGS += -DBUILD_TARGET=\"$(WAMR_BUILD_TARGET)\"
  INVOKE_NATIVE := invokeNative_arm.s
  AOT_RELOC := aot_reloc_arm.c
else ifeq ($(findstring THUMB,$(WAMR_BUILD_TARGET)), THUMB)
  CFLAGS += -DBUILD_TARGET_THUMB
  CFLAGS += -DBUILD_TARGET=\"$(WAMR_BUILD_TARGET)\"
  ifeq ($(CONFIG_ARCH_FPU),y)
  INVOKE_NATIVE := invokeNative_thumb_vfp.s
  else
  INVOKE_NATIVE := invokeNative_thumb.s
  endif
  AOT_RELOC := aot_reloc_thumb.c
else ifeq (${WAMR_BUILD_TARGET}, MIPS)
  CFLAGS += -DBUILD_TARGET_MIPS
  INVOKE_NATIVE := invokeNative_mips.s
  AOT_RELOC := aot_reloc_mips.c
else ifeq (${WAMR_BUILD_TARGET}, XTENSA)
  CFLAGS += -DBUILD_TARGET_XTENSA
  INVOKE_NATIVE := invokeNative_xtensa.s
  AOT_RELOC := aot_reloc_xtensa.c
else
  $(error Build target don't support)
endif

ifeq (${CONFIG_INTERPRETERS_WAMR_LOG},y)
CFLAGS += -DWASM_ENABLE_LOG=1
else
CFLAGS += -DWASM_ENABLE_LOG=0
endif

ifeq (${CONFIG_INTERPRETERS_WAMR_AOT},y)
CFLAGS += -I${IWASM_ROOT}/aot
CFLAGS += -DWASM_ENABLE_AOT=1
CSRCS += aot_loader.c \
         ${AOT_RELOC} \
         aot_runtime.c
else
CFLAGS += -DWASM_ENABLE_AOT=0
endif

CFLAGS += -DWASM_ENABLE_INTERP=1
CSRCS += wasm_runtime.c

ifeq (${CONFIG_INTERPRETERS_WAMR_FAST},y)
CFLAGS += -DWASM_ENABLE_FAST_INTERP=1
CSRCS += wasm_interp_fast.c
else
CSRCS += wasm_interp_classic.c
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_LIBC_BUILTIN),y)
CFLAGS += -DWASM_ENABLE_LIBC_BUILTIN=1
else
CFLAGS += -DWASM_ENABLE_LIBC_BUILTIN=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_MULTI_MODULE),y)
CFLAGS += -DWASM_ENABLE_MULTI_MODULE=1
else
CFLAGS += -DWASM_ENABLE_MULTI_MODULE=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_THREAD_MGR),y)
CFLAGS += -DWASM_ENABLE_THREAD_MGR=1
CSRCS += thread_manager.c
VPATH += ${IWASM_ROOT}/libraries/thread-mgr
else
CFLAGS += -DWASM_ENABLE_THREAD_MGR=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_MINILOADER),y)
CFLAGS += -DWASM_ENABLE_MINI_LOADER=1
CSRCS += wasm_mini_loader.c
else
CFLAGS += -DWASM_ENABLE_MINI_LOADER=0
CSRCS += wasm_loader.c
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_DISABLE_HW_BOUND_CHECK),y)
CFLAGS += -DWASM_DISABLE_HW_BOUND_CHECK=1
else
CFLAGS += -DWASM_DISABLE_HW_BOUND_CHECK=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_CUSTOM_NAME_SECTIONS),y)
CFLAGS += -DWASM_ENABLE_CUSTOM_NAME_SECTION=1
else
CFLAGS += -DWASM_ENABLE_CUSTOM_NAME_SECTION=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_GLOBAL_HEAP_POOL),y)
CFLAGS += -DWASM_ENABLE_GLOBAL_HEAP_POOL=1
CFLAGS += -DWASM_GLOBAL_HEAP_SIZE=$(CONFIG_INTERPRETERS_WAMR_GLOBAL_HEAP_POOL_SIZE)
else
CFLAGS += -DWASM_ENABLE_GLOBAL_HEAP_POOL=0
endif

CFLAGS += -DBH_ENABLE_MEMORY_PROFILING=0

CFLAGS += -Wno-strict-prototypes -Wno-shadow -Wno-unused-variable
CFLAGS += -Wno-int-conversion -Wno-implicit-function-declaration

CFLAGS += -I${CORE_ROOT} \
		      -I${IWASM_ROOT}/include \
          -I${IWASM_ROOT}/common \
          -I${SHARED_ROOT}/include \
          -I${SHARED_ROOT}/platform/include \
          -I${SHARED_ROOT}/utils \
          -I${SHARED_ROOT}/utils/uncommon \
          -I${SHARED_ROOT}/mem-alloc \
          -I${SHARED_ROOT}/platform/nuttx


ifeq (${WAMR_BUILD_INTERP}, 1)
CFLAGS += -I${IWASM_ROOT}/interpreter
endif

CSRCS += nuttx_platform.c \
         nuttx_thread.c \
         mem_alloc.c \
         ems_kfc.c \
         ems_alloc.c \
         ems_hmu.c \
         bh_assert.c \
         bh_common.c \
         bh_hashmap.c \
         bh_list.c \
         bh_log.c \
         bh_queue.c \
         bh_vector.c \
         bh_read_file.c \
         runtime_timer.c \
         libc_builtin_wrapper.c \
         wasm_runtime_common.c \
         wasm_native.c \
         wasm_exec_env.c \
         wasm_memory.c

ASRCS += ${INVOKE_NATIVE}

VPATH += ${SHARED_ROOT}/platform/nuttx
VPATH += ${SHARED_ROOT}/mem-alloc
VPATH += ${SHARED_ROOT}/mem-alloc/ems
VPATH += ${SHARED_ROOT}/utils
VPATH += ${SHARED_ROOT}/utils/uncommon
VPATH += ${IWASM_ROOT}/common
VPATH += ${IWASM_ROOT}/interpreter
VPATH += ${IWASM_ROOT}/libraries
VPATH += ${IWASM_ROOT}/libraries/libc-builtin
VPATH += ${IWASM_ROOT}/common/arch
VPATH += ${IWASM_ROOT}/aot
VPATH += ${IWASM_ROOT}/aot/arch
