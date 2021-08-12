/*
 * Copyright (C) 2021 XiaoMi Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_INTRINSIC_H
#define _AOT_INTRINSIC_H

#if WASM_ENABLE_WAMR_COMPILER != 0
#include "aot_llvm.h"
#endif

#include "aot_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AOT_INTRINSIC_GROUPS 2

/* Use uint64 as flag container:
 *   - The upper 16 bits are the intrinsic group number
 *   - The lower 48 bits are the intrinsic capability mask
 */

#define AOT_INTRINSIC_FLAG(group, value)                                      \
    ((((uint64)(group & 0xffffLL)) << 48) | (uint64)value)

#define AOT_INTRINSIC_FLAG_MASK (0x0000ffffffffffffLL)

#define AOT_INTRINSIC_GET_GROUP_FROM_FLAG(flag)                               \
    ((((uint64)flag) >> 48) & 0xffffLL)

#define AOT_INTRINSIC_FLAG_F32_FADD     AOT_INTRINSIC_FLAG(0, 0x000000000001)
#define AOT_INTRINSIC_FLAG_F32_FSUB     AOT_INTRINSIC_FLAG(0, 0x000000000002)
#define AOT_INTRINSIC_FLAG_F32_FMUL     AOT_INTRINSIC_FLAG(0, 0x000000000004)
#define AOT_INTRINSIC_FLAG_F32_FDIV     AOT_INTRINSIC_FLAG(0, 0x000000000008)
#define AOT_INTRINSIC_FLAG_F32_FABS     AOT_INTRINSIC_FLAG(0, 0x000000000010)
#define AOT_INTRINSIC_FLAG_F32_CEIL     AOT_INTRINSIC_FLAG(0, 0x000000000020)
#define AOT_INTRINSIC_FLAG_F32_FLOOR    AOT_INTRINSIC_FLAG(0, 0x000000000040)
#define AOT_INTRINSIC_FLAG_F32_TRUNC    AOT_INTRINSIC_FLAG(0, 0x000000000080)
#define AOT_INTRINSIC_FLAG_F32_RINT     AOT_INTRINSIC_FLAG(0, 0x000000000100)
#define AOT_INTRINSIC_FLAG_F32_SQRT     AOT_INTRINSIC_FLAG(0, 0x000000000200)
#define AOT_INTRINSIC_FLAG_F32_COPYSIGN AOT_INTRINSIC_FLAG(0, 0x000000000400)
#define AOT_INTRINSIC_FLAG_F32_MIN      AOT_INTRINSIC_FLAG(0, 0x000000000800)
#define AOT_INTRINSIC_FLAG_F32_MAX      AOT_INTRINSIC_FLAG(0, 0x000000001000)
#define AOT_INTRINSIC_FLAG_I32_CLZ      AOT_INTRINSIC_FLAG(0, 0x000000002000)
#define AOT_INTRINSIC_FLAG_I32_CTZ      AOT_INTRINSIC_FLAG(0, 0x000000004000)
#define AOT_INTRINSIC_FLAG_I32_POPCNT   AOT_INTRINSIC_FLAG(0, 0x000000008000)

#define AOT_INTRINSIC_FLAG_F64_FADD     AOT_INTRINSIC_FLAG(1, 0x000000000001)
#define AOT_INTRINSIC_FLAG_F64_FSUB     AOT_INTRINSIC_FLAG(1, 0x000000000002)
#define AOT_INTRINSIC_FLAG_F64_FMUL     AOT_INTRINSIC_FLAG(1, 0x000000000004)
#define AOT_INTRINSIC_FLAG_F64_FDIV     AOT_INTRINSIC_FLAG(1, 0x000000000008)
#define AOT_INTRINSIC_FLAG_F64_FABS     AOT_INTRINSIC_FLAG(1, 0x000000000010)
#define AOT_INTRINSIC_FLAG_F64_CEIL     AOT_INTRINSIC_FLAG(1, 0x000000000020)
#define AOT_INTRINSIC_FLAG_F64_FLOOR    AOT_INTRINSIC_FLAG(1, 0x000000000040)
#define AOT_INTRINSIC_FLAG_F64_TRUNC    AOT_INTRINSIC_FLAG(1, 0x000000000080)
#define AOT_INTRINSIC_FLAG_F64_RINT     AOT_INTRINSIC_FLAG(1, 0x000000000100)
#define AOT_INTRINSIC_FLAG_F64_SQRT     AOT_INTRINSIC_FLAG(1, 0x000000000200)
#define AOT_INTRINSIC_FLAG_F64_COPYSIGN AOT_INTRINSIC_FLAG(1, 0x000000000400)
#define AOT_INTRINSIC_FLAG_F64_MIN      AOT_INTRINSIC_FLAG(1, 0x000000000800)
#define AOT_INTRINSIC_FLAG_F64_MAX      AOT_INTRINSIC_FLAG(1, 0x000000001000)
#define AOT_INTRINSIC_FLAG_I64_CLZ      AOT_INTRINSIC_FLAG(1, 0x000000002000)
#define AOT_INTRINSIC_FLAG_I64_CTZ      AOT_INTRINSIC_FLAG(1, 0x000000004000)
#define AOT_INTRINSIC_FLAG_I64_POPCNT   AOT_INTRINSIC_FLAG(1, 0x000000008000)

float32
aot_intrinsic_fadd_f32(float32 a, float32 b);

float64
aot_intrinsic_fadd_f64(float64 a, float64 b);

float32
aot_intrinsic_fsub_f32(float32 a, float32 b);

float64
aot_intrinsic_fsub_f64(float64 a, float64 b);

float32
aot_intrinsic_fmul_f32(float32 a, float32 b);

float64
aot_intrinsic_fmul_f64(float64 a, float64 b);

float32
aot_intrinsic_fdiv_f32(float32 a, float32 b);

float64
aot_intrinsic_fdiv_f64(float64 a, float64 b);

float32
aot_intrinsic_fabs_f32(float32 a);

float64
aot_intrinsic_fabs_f64(float64 a);

float32
aot_intrinsic_ceil_f32(float32 a);

float64
aot_intrinsic_ceil_f64(float64 a);

float32
aot_intrinsic_floor_f32(float32 a);

float64
aot_intrinsic_floor_f64(float64 a);

float32
aot_intrinsic_trunc_f32(float32 a);

float64
aot_intrinsic_trunc_f64(float64 a);

float32
aot_intrinsic_rint_f32(float32 a);

float64
aot_intrinsic_rint_f64(float64 a);

float32
aot_intrinsic_sqrt_f32(float32 a);

float64
aot_intrinsic_sqrt_f64(float64 a);

float32
aot_intrinsic_copysign_f32(float32 a, float32 b);

float64
aot_intrinsic_copysign_f64(float64 a, float64 b);

float32
aot_intrinsic_fmin_f32(float32 a, float32 b);

float64
aot_intrinsic_fmin_f64(float64 a, float64 b);

float32
aot_intrinsic_fmax_f32(float32 a, float32 b);

float64
aot_intrinsic_fmax_f64(float64 a, float64 b);

uint32
aot_intrinsic_clz_i32(uint32 type);

uint32
aot_intrinsic_clz_i64(uint64 type);

uint32
aot_intrinsic_ctz_i32(uint32 type);

uint32
aot_intrinsic_ctz_i64(uint64 type);

uint32
aot_intrinsic_popcnt_i32(uint32 u);

uint32
aot_intrinsic_popcnt_i64(uint64 u);

const char *
aot_intrinsic_get_symbol(const char *llvm_intrinsic);

#if WASM_ENABLE_WAMR_COMPILER != 0 || WASM_ENABLE_JIT != 0
bool
aot_intrinsic_check_capability(const AOTCompContext *comp_ctx,
                               const char *llvm_intrinsic);

void
aot_intrinsic_fill_capability_flags(AOTCompContext *comp_ctx);
#endif

#ifdef __cplusplus
}
#endif

#endif /* end of _AOT_INTRINSIC_H */
