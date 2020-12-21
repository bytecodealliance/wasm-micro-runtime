/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_CONVERSIONS_H_
#define _SIMD_CONVERSIONS_H_

#include "../aot_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
aot_compile_simd_i8x16_narrow_i16x8(AOTCompContext *comp_ctx,
                                    AOTFuncContext *func_ctx,
                                    bool is_signed);

bool
aot_compile_simd_i16x8_narrow_i32x4(AOTCompContext *comp_ctx,
                                    AOTFuncContext *func_ctx,
                                    bool is_signed);

bool
aot_compile_simd_i16x8_widen_i8x16(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx,
                                   bool is_low,
                                   bool is_signed);

bool
aot_compile_simd_i32x4_widen_i16x8(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx,
                                   bool is_low,
                                   bool is_signed);

bool
aot_compile_simd_i32x4_trunc_sat_f32x4(AOTCompContext *comp_ctx,
                                       AOTFuncContext *func_ctx,
                                       bool is_signed);

bool
aot_compile_simd_f32x4_convert_i32x4(AOTCompContext *comp_ctx,
                                     AOTFuncContext *func_ctx,
                                     bool is_signed);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_CONVERSIONS_H_ */
