/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _PLATFORM_INTERNAL_H
#define _PLATFORM_INTERNAL_H

#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BH_PLATFORM_VXWORKS
#define BH_PLATFORM_VXWORKS
#endif

/* Stack size of applet threads's native part.  */
#define BH_APPLET_PRESERVED_STACK_SIZE (32 * 1024)

/* Default thread priority */
#define BH_THREAD_DEFAULT_PRIORITY 0

typedef pthread_t korp_tid;
typedef pthread_mutex_t korp_mutex;
typedef pthread_cond_t korp_cond;
typedef pthread_t korp_thread;
typedef sem_t korp_sem;

#define OS_THREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define os_thread_local_attribute __thread

#if WASM_DISABLE_BLOCK_INSN_INTERRUPT == 0
#define OS_ENABLE_BLOCK_INSN_INTERRUPT

typedef void (*os_block_insn_sig_handler)();
bool
os_interrupt_block_insn_init(os_block_insn_sig_handler handler);
#endif /* WASM_DISABLE_BLOCK_INSN_INTERRUPT */

#if WASM_DISABLE_HW_BOUND_CHECK == 0
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64) \
    || defined(BUILD_TARGET_AARCH64)

#define OS_ENABLE_HW_BOUND_CHECK

#define os_alloca alloca

#define os_getpagesize getpagesize

typedef void (*os_signal_handler)(void *sig_addr);

int
os_thread_signal_init(os_signal_handler handler);

void
os_thread_signal_destroy();

bool
os_thread_signal_inited();

void
os_signal_unmask();

void
os_sigreturn();
#endif /* end of BUILD_TARGET_X86_64/AMD_64/AARCH64 */
#endif /* end of WASM_DISABLE_HW_BOUND_CHECK */

#if defined(OS_ENABLE_BLOCK_INSN_INTERRUPT) || defined(OS_ENABLE_HW_BOUND_CHECK)
#include <setjmp.h>
typedef jmp_buf korp_jmpbuf;
#define os_setjmp setjmp
#define os_longjmp longjmp
#endif

#ifdef __cplusplus
}
#endif

#endif /* end of _PLATFORM_INTERNAL_H */
