/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_BIT_SHIFTS_H_
#define _SIMD_BIT_SHIFTS_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_i8x16_shift(JitCompContext *cc, IntShift shift_op);

bool
jit_compile_simd_i16x8_shift(JitCompContext *cc, IntShift shift_op);

bool
jit_compile_simd_i32x4_shift(JitCompContext *cc, IntShift shift_op);

bool
jit_compile_simd_i64x2_shift(JitCompContext *cc, IntShift shift_op);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_BIT_SHIFTS_H_ */
