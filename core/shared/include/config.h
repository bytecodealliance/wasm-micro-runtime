/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#if !defined(BUILD_TARGET_X86_64) \
    && !defined(BUILD_TARGET_AMD_64) \
    && !defined(BUILD_TARGET_X86_32) \
    && !defined(BUILD_TARGET_ARM) \
    && !defined(BUILD_TARGET_ARM_VFP) \
    && !defined(BUILD_TARGET_THUMB) \
    && !defined(BUILD_TARGET_THUMB_VFP) \
    && !defined(BUILD_TARGET_MIPS) \
    && !defined(BUILD_TARGET_XTENSA)
#if defined(__x86_64__) || defined(__x86_64)
#define BUILD_TARGET_X86_64
#elif defined(__amd64__) || defined(__amd64)
#define BUILD_TARGET_AMD_64
#elif defined(__i386__) || defined(__i386) || defined(i386)
#define BUILD_TARGET_X86_32
#elif defined(__thumb__)
#define BUILD_TARGET_THUMB
#define BUILD_TARGET "THUMBV4T"
#elif defined(__arm__)
#define BUILD_TARGET_ARM
#define BUILD_TARGET "ARMV4T"
#elif defined(__mips__) || defined(__mips) || defined(mips)
#define BUILD_TARGET_MIPS
#elif defined(__XTENSA__)
#define BUILD_TARGET_XTENSA
#else
#error "Build target isn't set"
#endif
#endif

enum {
    /* Memory allocator ems */
    MEM_ALLOCATOR_EMS = 0,
    /* Memory allocator tlsf */
    MEM_ALLOCATOR_TLSF
};

/* Default memory allocator */
#define DEFAULT_MEM_ALLOCATOR MEM_ALLOCATOR_EMS

#ifndef WASM_ENABLE_INTERP
#define WASM_ENABLE_INTERP 0
#endif

#ifndef WASM_ENABLE_AOT
#define WASM_ENABLE_AOT 0
#endif

#ifndef WASM_ENABLE_JIT
#define WASM_ENABLE_JIT 0
#endif

#if (WASM_ENABLE_AOT == 0) && (WASM_ENABLE_JIT != 0)
/* JIT can only be enabled when AOT is enabled */
#undef WASM_ENABLE_JIT
#define WASM_ENABLE_JIT 0
#endif

#ifndef WASM_ENABLE_WAMR_COMPILER
#define WASM_ENABLE_WAMR_COMPILER 0
#endif

#ifndef WASM_ENABLE_LIBC_BUILTIN
#define WASM_ENABLE_LIBC_BUILTIN 0
#endif

#ifndef WASM_ENABLE_LIBC_WASI
#define WASM_ENABLE_LIBC_WASI 0
#endif

#ifndef WASM_ENABLE_BASE_LIB
#define WASM_ENABLE_BASE_LIB 0
#endif

/* WASM log system */
#ifndef WASM_ENABLE_LOG
#define WASM_ENABLE_LOG 1
#endif

#if defined(BUILD_TARGET_X86_32) || defined(BUILD_TARGET_X86_64)
#define WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS 1
#else
#define WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS 0
#endif

/* WASM Interpreter labels-as-values feature */
#define WASM_ENABLE_LABELS_AS_VALUES 1

/* Heap and stack profiling */
#define BEIHAI_ENABLE_MEMORY_PROFILING 0

/* Max app number of all modules */
#define MAX_APP_INSTALLATIONS 3

/* Default timer number in one app */
#define DEFAULT_TIMERS_PER_APP 20

/* Max timer number in one app */
#define MAX_TIMERS_PER_APP 30

/* Max connection number in one app */
#define MAX_CONNECTION_PER_APP 20

/* Max resource registration number in one app */
#define RESOURCE_REGISTRATION_NUM_MAX 16

/* Max length of resource/event url */
#define RESOUCE_EVENT_URL_LEN_MAX 256

/* Default length of queue */
#define DEFAULT_QUEUE_LENGTH 50

/* Default watchdog interval in ms */
#define DEFAULT_WATCHDOG_INTERVAL (3 * 60 * 1000)

/* Support memory.grow opcode and enlargeMemory function */
#define WASM_ENABLE_MEMORY_GROW 1

/* The max percentage of global heap that app memory space can grow */
#define APP_MEMORY_MAX_GLOBAL_HEAP_PERCENT 1 / 3

/* Default base offset of app heap space */
#define DEFAULT_APP_HEAP_BASE_OFFSET (1 * BH_GB)

/* Default min/max heap size of each app */
#define APP_HEAP_SIZE_DEFAULT (8 * 1024)
#define APP_HEAP_SIZE_MIN (2 * 1024)
#define APP_HEAP_SIZE_MAX (1024 * 1024)

/* Default wasm stack size of each app */
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
#define DEFAULT_WASM_STACK_SIZE (16 * 1024)
#else
#define DEFAULT_WASM_STACK_SIZE (12 * 1024)
#endif

/* Default/min/max stack size of each app thread */
#if !defined(BH_PLATFORM_ZEPHYR) && !defined(BH_PLATFORM_ALIOS_THINGS)
#define APP_THREAD_STACK_SIZE_DEFAULT (20 * 1024)
#define APP_THREAD_STACK_SIZE_MIN (16 * 1024)
#define APP_THREAD_STACK_SIZE_MAX (256 * 1024)
#else
#define APP_THREAD_STACK_SIZE_DEFAULT (4 * 1024)
#define APP_THREAD_STACK_SIZE_MIN (2 * 1024)
#define APP_THREAD_STACK_SIZE_MAX (256 * 1024)
#endif

/* Default wasm block address cache size and conflict list size */
#define BLOCK_ADDR_CACHE_SIZE 64
#define BLOCK_ADDR_CONFLICT_SIZE 2

#endif /* end of _CONFIG_H_ */

