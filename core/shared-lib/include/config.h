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
    && !defined(BUILD_TARGET_THUMB) \
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

/* Memory allocator ems */
#define MEM_ALLOCATOR_EMS 0

/* Memory allocator tlsf */
#define MEM_ALLOCATOR_TLSF 1

/* Default memory allocator */
#define DEFAULT_MEM_ALLOCATOR MEM_ALLOCATOR_EMS

/* Beihai log system */
#define BEIHAI_ENABLE_LOG 1

/* Beihai debugger support */
#define BEIHAI_ENABLE_TOOL_AGENT 1

/* Beihai debug monitoring server, must define
 BEIHAI_ENABLE_TOOL_AGENT firstly */
#define BEIHAI_ENABLE_TOOL_AGENT_BDMS 1

/* enable no signature on sdv since verify doesn't work as lacking public key */
#ifdef CONFIG_SDV
#define BEIHAI_ENABLE_NO_SIGNATURE 1
#else
#define BEIHAI_ENABLE_NO_SIGNATURE 0
#endif

/* WASM VM log system */
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

/* Workflow heap size */
/*
#define WORKING_FLOW_HEAP_SIZE 0
*/

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
#define DEFAULT_WASM_STACK_SIZE (12 * 1024)
#else
#define DEFAULT_WASM_STACK_SIZE (8 * 1024)
#endif

/* Default/min/max stack size of each app thread */
#ifndef __ZEPHYR__
#define APP_THREAD_STACK_SIZE_DEFAULT (20 * 1024)
#define APP_THREAD_STACK_SIZE_MIN (16 * 1024)
#define APP_THREAD_STACK_SIZE_MAX (256 * 1024)
#else
#define APP_THREAD_STACK_SIZE_DEFAULT (4 * 1024)
#define APP_THREAD_STACK_SIZE_MIN (2 * 1024)
#define APP_THREAD_STACK_SIZE_MAX (256 * 1024)
#endif

/* External memory space provided by user,
   but not wasm memory space and app heap space */
#ifndef WASM_ENABLE_EXT_MEMORY_SPACE
#define WASM_ENABLE_EXT_MEMORY_SPACE 0
#endif

/* Default base offset of external memory space */
#define DEFAULT_EXT_MEM_BASE_OFFSET (-2 * BH_GB)

#ifndef bh_printf
#define bh_printf printf
#endif

#ifndef WASM_ENABLE_GUI
#define WASM_ENABLE_GUI 0
#endif

#endif /* end of _CONFIG_H_ */

