/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_ACCESS_LANES_H_
#define _SIMD_ACCESS_LANES_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_shuffle(JitCompContext *cc, const uint8 *frame_ip);

bool
jit_compile_simd_swizzle(JitCompContext *cc);

bool
jit_compile_simd_extract_i8x16(JitCompContext *cc, uint8 lane_id,
                               bool is_signed);

bool
jit_compile_simd_extract_i16x8(JitCompContext *cc, uint8 lane_id,
                               bool is_signed);

bool
jit_compile_simd_extract_i32x4(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_extract_i64x2(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_extract_f32x4(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_extract_f64x2(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_replace_i8x16(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_replace_i16x8(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_replace_i32x4(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_replace_i64x2(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_replace_f32x4(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_replace_f64x2(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_load8_lane(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_load16_lane(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_load32_lane(JitCompContext *cc, uint8 lane_id);

bool
jit_compile_simd_load64_lane(JitCompContext *cc, uint8 lane_id);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_ACCESS_LANES_H_ */
