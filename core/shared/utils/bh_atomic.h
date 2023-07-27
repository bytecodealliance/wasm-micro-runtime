/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_ATOMIC_H
#define _BH_ATOMIC_H

#include "gnuc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32 bh_atomic_32_t;

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
#define BH_ATOMIC_32_IS_ATOMIC 1
#define BH_ATOMIC_32_LOAD(v) __atomic_load_n(&(v), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_32_FETCH_OR(v, val) \
    __atomic_fetch_or(&(v), (val), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_32_FETCH_AND(v, val) \
    __atomic_fetch_and(&(v), (val), __ATOMIC_SEQ_CST)
#else /* else of defined(CLANG_GCC_HAS_ATOMIC_BUILTIN) */
#define BH_ATOMIC_32_LOAD(v) (v)
#define BH_ATOMIC_32_FETCH_OR(v, val) ((v) |= (val))
#define BH_ATOMIC_32_FETCH_AND(v, val) ((v) &= (val))

/* The flag can be defined by the user if the platform
   supports atomic access to uint32 aligned memory. */
#ifdef WASM_UINT32_IS_ATOMIC
#define BH_ATOMIC_32_IS_ATOMIC 1
#else /* else of WASM_UINT32_IS_ATOMIC */
#define BH_ATOMIC_32_IS_ATOMIC 0
#endif /* WASM_UINT32_IS_ATOMIC */

#endif

#ifdef __cplusplus
}
#endif

#endif /* end of _BH_ATOMIC_H */
