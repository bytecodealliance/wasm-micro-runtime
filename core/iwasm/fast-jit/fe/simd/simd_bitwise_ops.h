/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_BITWISE_OPS_H_
#define _SIMD_BITWISE_OPS_H_

#include "../../jit_compiler.h"
#include "../../jit_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compile_simd_v128_bitwise(JitCompContext *cc, V128Bitwise bitwise_op);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _SIMD_BITWISE_OPS_H_ */
