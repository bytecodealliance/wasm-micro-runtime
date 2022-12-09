/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_SAT_INT_ARITH_H_
#define _SIMD_SAT_INT_ARITH_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_i8x16_saturate(JitCompContext *cc, V128Arithmetic arith_op,
                                bool is_signed);

bool
jit_compile_simd_i16x8_saturate(JitCompContext *cc, V128Arithmetic arith_op,
                                bool is_signed);

bool
jit_compile_simd_i32x4_saturate(JitCompContext *cc, V128Arithmetic arith_op,
                                bool is_signed);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_SAT_INT_ARITH_H_ */
