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

/*
 * Why don't we use C11 stdatomics here?
 *
 * Unlike C11 stdatomics,
 *
 * - bh_atomic_xxx_t is guaranteed to have the same size as the base type.
 *   Thus more friendly to our AOT conventions.
 *
 * - It's available for C++.
 *   Although C++23 will have C-compatible stdatomics.h, it isn't widely
 *   available yet.
 */

/*
 * Note about BH_ATOMIC_32_IS_ATOMIC
 *
 * If BH_ATOMIC_32_IS_ATOMIC == 0, BH_ATOMIC_xxx operations defined below
 * are not really atomic and require an external lock.
 *
 * Expected usage is:
 *
 *     bh_atomic_32_t var = 0;
 *     uint32 old;
 * #if BH_ATOMIC_32_IS_ATOMIC == 0
 *     lock(&some_lock);
 * #endif
 *     old = BH_ATOMIC_32_FETCH_AND(var, 1);
 * #if BH_ATOMIC_32_IS_ATOMIC == 0
 *     unlock(&some_lock);
 * #endif
 */

typedef uint32 bh_atomic_32_t;
typedef uint16 bh_atomic_16_t;

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
#define BH_ATOMIC_32_STORE(v, val) __atomic_store_n(&(v), val, __ATOMIC_SEQ_CST)
#define BH_ATOMIC_32_FETCH_OR(v, val) \
    __atomic_fetch_or(&(v), (val), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_32_FETCH_AND(v, val) \
    __atomic_fetch_and(&(v), (val), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_32_FETCH_ADD(v, val) \
    __atomic_fetch_add(&(v), (val), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_32_FETCH_SUB(v, val) \
    __atomic_fetch_sub(&(v), (val), __ATOMIC_SEQ_CST)

#define BH_ATOMIC_16_IS_ATOMIC 1
#define BH_ATOMIC_16_LOAD(v) __atomic_load_n(&(v), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_16_STORE(v, val) __atomic_store_n(&(v), val, __ATOMIC_SEQ_CST)
#define BH_ATOMIC_16_FETCH_OR(v, val) \
    __atomic_fetch_or(&(v), (val), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_16_FETCH_AND(v, val) \
    __atomic_fetch_and(&(v), (val), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_16_FETCH_ADD(v, val) \
    __atomic_fetch_add(&(v), (val), __ATOMIC_SEQ_CST)
#define BH_ATOMIC_16_FETCH_SUB(v, val) \
    __atomic_fetch_sub(&(v), (val), __ATOMIC_SEQ_CST)
#else /* else of defined(CLANG_GCC_HAS_ATOMIC_BUILTIN) */
#define BH_ATOMIC_32_LOAD(v) (v)
#define BH_ATOMIC_32_STORE(v, val) (v) = val
#define BH_ATOMIC_32_FETCH_OR(v, val) nonatomic_32_fetch_or(&(v), val)
#define BH_ATOMIC_32_FETCH_AND(v, val) nonatomic_32_fetch_and(&(v), val)
#define BH_ATOMIC_32_FETCH_ADD(v, val) nonatomic_32_fetch_add(&(v), val)
#define BH_ATOMIC_32_FETCH_SUB(v, val) nonatomic_32_fetch_sub(&(v), val)

#define BH_ATOMIC_16_LOAD(v) (v)
#define BH_ATOMIC_16_STORE(v) (v) = val
#define BH_ATOMIC_16_FETCH_OR(v, val) nonatomic_16_fetch_or(&(v), val)
#define BH_ATOMIC_16_FETCH_AND(v, val) nonatomic_16_fetch_and(&(v), val)
#define BH_ATOMIC_16_FETCH_ADD(v, val) nonatomic_16_fetch_add(&(v), val)
#define BH_ATOMIC_16_FETCH_SUB(v, val) nonatomic_16_fetch_sub(&(v), val)

static inline uint32
nonatomic_32_fetch_or(bh_atomic_32_t *p, uint32 val)
{
    uint32 old = *p;
    *p |= val;
    return old;
}

static inline uint32
nonatomic_32_fetch_and(bh_atomic_32_t *p, uint32 val)
{
    uint32 old = *p;
    *p &= val;
    return old;
}

static inline uint32
nonatomic_32_fetch_add(bh_atomic_32_t *p, uint32 val)
{
    uint32 old = *p;
    *p += val;
    return old;
}

static inline uint32
nonatomic_32_fetch_sub(bh_atomic_32_t *p, uint32 val)
{
    uint32 old = *p;
    *p -= val;
    return old;
}

static inline uint16
nonatomic_16_fetch_or(bh_atomic_16_t *p, uint16 val)
{
    uint16 old = *p;
    *p |= val;
    return old;
}

static inline uint16
nonatomic_16_fetch_and(bh_atomic_16_t *p, uint16 val)
{
    uint16 old = *p;
    *p &= val;
    return old;
}

static inline uint16
nonatomic_16_fetch_add(bh_atomic_16_t *p, uint16 val)
{
    uint16 old = *p;
    *p += val;
    return old;
}

static inline uint16
nonatomic_16_fetch_sub(bh_atomic_16_t *p, uint16 val)
{
    uint16 old = *p;
    *p -= val;
    return old;
}

/* The flag can be defined by the user if the platform
   supports atomic access to uint32 aligned memory. */
#ifdef WASM_UINT32_IS_ATOMIC
#define BH_ATOMIC_32_IS_ATOMIC 1
#else /* else of WASM_UINT32_IS_ATOMIC */
#define BH_ATOMIC_32_IS_ATOMIC 0
#endif /* WASM_UINT32_IS_ATOMIC */

#ifdef WASM_UINT16_IS_ATOMIC
#define BH_ATOMIC_16_IS_ATOMIC 1
#else /* else of WASM_UINT16_IS_ATOMIC */
#define BH_ATOMIC_16_IS_ATOMIC 0
#endif /* WASM_UINT16_IS_ATOMIC */

#endif

#ifdef __cplusplus
}
#endif

#endif /* end of _BH_ATOMIC_H */
