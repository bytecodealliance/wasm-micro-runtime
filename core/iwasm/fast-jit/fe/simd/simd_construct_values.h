/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_CONSTRUCT_VALUES_H_
#define _SIMD_CONSTRUCT_VALUES_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_v128_const(JitCompContext *cc, const uint8 *imm_bytes);

bool
jit_compile_simd_splat(JitCompContext *cc, uint8 splat_opcode);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_CONSTRUCT_VALUES_H_ */
