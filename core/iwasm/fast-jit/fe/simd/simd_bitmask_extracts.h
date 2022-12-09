/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_BITMASK_EXTRACTS_H_
#define _SIMD_BITMASK_EXTRACTS_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_i8x16_bitmask(JitCompContext *cc);

bool
jit_compile_simd_i16x8_bitmask(JitCompContext *cc);

bool
jit_compile_simd_i32x4_bitmask(JitCompContext *cc);

bool
jit_compile_simd_i64x2_bitmask(JitCompContext *cc);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_BITMASK_EXTRACTS_H_ */
