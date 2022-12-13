/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_INT_ARITH_H_
#define _SIMD_INT_ARITH_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_i8x16_arith(JitCompContext *cc, V128Arithmetic arith_op);

bool
jit_compile_simd_i16x8_arith(JitCompContext *cc, V128Arithmetic arith_op);

bool
jit_compile_simd_i32x4_arith(JitCompContext *cc, V128Arithmetic arith_op);

bool
jit_compile_simd_i64x2_arith(JitCompContext *cc, V128Arithmetic arith_op);

bool
jit_compile_simd_i8x16_neg(JitCompContext *cc);

bool
jit_compile_simd_i16x8_neg(JitCompContext *cc);

bool
jit_compile_simd_i32x4_neg(JitCompContext *cc);

bool
jit_compile_simd_i64x2_neg(JitCompContext *cc);

bool
jit_compile_simd_i8x16_popcnt(JitCompContext *cc);

bool
jit_compile_simd_i8x16_cmp(JitCompContext *cc, V128Arithmetic arith_op,
                           bool is_signed);

bool
jit_compile_simd_i16x8_cmp(JitCompContext *cc, V128Arithmetic arith_op,
                           bool is_signed);

bool
jit_compile_simd_i32x4_cmp(JitCompContext *cc, V128Arithmetic arith_op,
                           bool is_signed);

bool
jit_compile_simd_i8x16_abs(JitCompContext *cc);

bool
jit_compile_simd_i16x8_abs(JitCompContext *cc);

bool
jit_compile_simd_i32x4_abs(JitCompContext *cc);

bool
jit_compile_simd_i64x2_abs(JitCompContext *cc);

bool
jit_compile_simd_i8x16_avgr_u(JitCompContext *cc);

bool
jit_compile_simd_i16x8_avgr_u(JitCompContext *cc);

bool
jit_compile_simd_i32x4_avgr_u(JitCompContext *cc);

bool
jit_compile_simd_i32x4_dot_i16x8(JitCompContext *cc);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_INT_ARITH_H_ */
