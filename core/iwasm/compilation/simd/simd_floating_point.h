/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_FLOATING_POINT_H_
#define _SIMD_FLOATING_POINT_H_

#include "../aot_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
aot_compile_simd_f32x4_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             FloatArithmetic arith_op);

bool
aot_compile_simd_f64x2_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             FloatArithmetic arith_op);

bool
aot_compile_simd_f32x4_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

bool
aot_compile_simd_f64x2_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

bool
aot_compile_simd_f32x4_abs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

bool
aot_compile_simd_f64x2_abs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

bool
aot_compile_simd_f32x4_sqrt(AOTCompContext *comp_ctx,
                            AOTFuncContext *func_ctx);

bool
aot_compile_simd_f64x2_sqrt(AOTCompContext *comp_ctx,
                            AOTFuncContext *func_ctx);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_FLOATING_POINT_H_ */
