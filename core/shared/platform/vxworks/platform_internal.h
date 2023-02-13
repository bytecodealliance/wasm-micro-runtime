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

#if WASM_DISABLE_HW_BOUND_CHECK == 0
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)            \
    || defined(BUILD_TARGET_AARCH64) || defined(BUILD_TARGET_RISCV64_LP64D) \
    || defined(BUILD_TARGET_RISCV64_LP64)

#define OS_ENABLE_HW_BOUND_CHECK

#define os_alloca alloca

#define os_getpagesize getpagesize

#endif /* end of BUILD_TARGET_X86_64/AMD_64/AARCH64/RISCV64 */
#endif /* end of WASM_DISABLE_HW_BOUND_CHECK */

#if WASM_ENABLE_INTERRUPT_BLOCK_INSN != 0 && WASM_ENABLE_THREAD_MGR != 0
#define OS_ENABLE_INTERRUPT_BLOCK_INSN
#endif

#if defined(OS_ENABLE_HW_BOUND_CHECK) || defined(OS_ENABLE_INTERRUPT_BLOCK_INSN)

#define OS_SIGSEGV SIGSEGV
#define OS_SIGBUS SIGBUS
#define OS_SIGUSR1 SIGUSR1

typedef void (*os_signal_handler)(int sig, void *sig_addr);

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

#if defined(OS_ENABLE_INTERRUPT_BLOCK_INSN)
void
os_thread_set_interruptable(bool flag);
#endif

#include <setjmp.h>
typedef jmp_buf korp_jmpbuf;
#define os_setjmp setjmp
#define os_longjmp longjmp

#endif /* end of defined(OS_ENABLE_HW_BOUND_CHECK) || \
          defined(OS_ENABLE_INTERRUPT_BLOCK_INSN) */

#ifdef __cplusplus
}
#endif

#endif /* end of _PLATFORM_INTERNAL_H */
