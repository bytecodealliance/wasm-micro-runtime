/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_FLOATING_POINT_H_
#define _SIMD_FLOATING_POINT_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_f32x4_arith(JitCompContext *cc, FloatArithmetic arith_op);

bool
jit_compile_simd_f64x2_arith(JitCompContext *cc, FloatArithmetic arith_op);

bool
jit_compile_simd_f32x4_neg(JitCompContext *cc);

bool
jit_compile_simd_f64x2_neg(JitCompContext *cc);

bool
jit_compile_simd_f32x4_abs(JitCompContext *cc);

bool
jit_compile_simd_f64x2_abs(JitCompContext *cc);

bool
jit_compile_simd_f32x4_round(JitCompContext *cc);

bool
jit_compile_simd_f64x2_round(JitCompContext *cc);

bool
jit_compile_simd_f32x4_sqrt(JitCompContext *cc);

bool
jit_compile_simd_f64x2_sqrt(JitCompContext *cc);

bool
jit_compile_simd_f32x4_ceil(JitCompContext *cc);

bool
jit_compile_simd_f64x2_ceil(JitCompContext *cc);

bool
jit_compile_simd_f32x4_floor(JitCompContext *cc);

bool
jit_compile_simd_f64x2_floor(JitCompContext *cc);

bool
jit_compile_simd_f32x4_trunc(JitCompContext *cc);

bool
jit_compile_simd_f64x2_trunc(JitCompContext *cc);

bool
jit_compile_simd_f32x4_nearest(JitCompContext *cc);

bool
jit_compile_simd_f64x2_nearest(JitCompContext *cc);

bool
jit_compile_simd_f32x4_min_max(JitCompContext *cc, bool run_min);

bool
jit_compile_simd_f64x2_min_max(JitCompContext *cc, bool run_min);

bool
jit_compile_simd_f32x4_pmin_pmax(JitCompContext *cc, bool run_min);

bool
jit_compile_simd_f64x2_pmin_pmax(JitCompContext *cc, bool run_min);

bool
jit_compile_simd_f64x2_demote(JitCompContext *cc);

bool
jit_compile_simd_f32x4_promote(JitCompContext *cc);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_FLOATING_POINT_H_ */
