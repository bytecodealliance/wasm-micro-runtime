/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_LOAD_STORE_H_
#define _SIMD_LOAD_STORE_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_v128_load(JitCompContext *cc, uint32 align, uint32 offset);

bool
jit_compile_simd_load_extend(JitCompContext *cc, uint8 opcode, uint32 align,
                             uint32 offset);

bool
jit_compile_simd_load_splat(JitCompContext *cc, uint8 opcode, uint32 align,
                            uint32 offset);

bool
jit_compile_simd_load_lane(JitCompContext *cc, uint8 opcode, uint32 align,
                           uint32 offset, uint8 lane_id);

bool
jit_compile_simd_load_zero(JitCompContext *cc, uint8 opcode, uint32 align,
                           uint32 offset);

bool
jit_compile_simd_v128_store(JitCompContext *cc, uint32 align, uint32 offset);

bool
jit_compile_simd_store_lane(JitCompContext *cc, uint8 opcode, uint32 align,
                            uint32 offset, uint8 lane_id);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_LOAD_STORE_H_ */
