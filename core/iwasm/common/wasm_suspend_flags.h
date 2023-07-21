/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_SUSPEND_FLAGS_H
#define _WASM_SUSPEND_FLAGS_H

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Need to terminate */
#define WASM_SUSPEND_FLAG_TERMINATE 0x1
/* Need to suspend */
#define WASM_SUSPEND_FLAG_SUSPEND 0x2
/* Need to go into breakpoint */
#define WASM_SUSPEND_FLAG_BREAKPOINT 0x4
/* Return from pthread_exit */
#define WASM_SUSPEND_FLAG_EXIT 0x8

typedef union WASMSuspendFlags {
    uint32 flags;
    uintptr_t __padding__;
} WASMSuspendFlags;

#if defined(__GNUC_PREREQ)
#if __GNUC_PREREQ(4, 7)
#define CLANG_GCC_HAS_ATOMIC_BUILTIN
#endif
#elif defined(__clang__)
#if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 0)
#define CLANG_GCC_HAS_ATOMIC_BUILTIN
#endif
#endif

#if defined(CLANG_GCC_HAS_ATOMIC_BUILTIN)
#define WASM_SUSPEND_FLAGS_IS_ATOMIC 1
#define WASM_SUSPEND_FLAGS_GET(s_flags) \
    __atomic_load_n(&s_flags.flags, __ATOMIC_SEQ_CST)
#define WASM_SUSPEND_FLAGS_FETCH_OR(s_flags, val) \
    __atomic_fetch_or(&s_flags.flags, val, __ATOMIC_SEQ_CST)
#define WASM_SUSPEND_FLAGS_FETCH_AND(s_flags, val) \
    __atomic_fetch_and(&s_flags.flags, val, __ATOMIC_SEQ_CST)
#else /* else of defined(CLANG_GCC_HAS_ATOMIC_BUILTIN) */
#define WASM_SUSPEND_FLAGS_GET(s_flags) (s_flags.flags)
#define WASM_SUSPEND_FLAGS_FETCH_OR(s_flags, val) (s_flags.flags |= val)
#define WASM_SUSPEND_FLAGS_FETCH_AND(s_flags, val) (s_flags.flags &= val)

/* The flag can be defined by the user if the platform
   supports atomic access to uint32 aligned memory. */
#ifdef WASM_UINT32_IS_ATOMIC
#define WASM_SUSPEND_FLAGS_IS_ATOMIC 1
#else /* else of WASM_UINT32_IS_ATOMIC */
#define WASM_SUSPEND_FLAGS_IS_ATOMIC 0
#endif /* WASM_UINT32_IS_ATOMIC */

#endif

#if WASM_SUSPEND_FLAGS_IS_ATOMIC != 0
#define WASM_SUSPEND_FLAGS_LOCK(lock) (void)0
#define WASM_SUSPEND_FLAGS_UNLOCK(lock) (void)0
#else /* else of WASM_SUSPEND_FLAGS_IS_ATOMIC */
#define WASM_SUSPEND_FLAGS_LOCK(lock) os_mutex_lock(&lock)
#define WASM_SUSPEND_FLAGS_UNLOCK(lock) os_mutex_unlock(&lock);
#endif /* WASM_SUSPEND_FLAGS_IS_ATOMIC */

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_SUSPEND_FLAGS_H */