/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _SIMD_COMMON_H_
#define _SIMD_COMMON_H_

#include "../aot_compiler.h"

LLVMValueRef
simd_pop_v128_and_bitcast(const AOTCompContext *comp_ctx,
                          const AOTFuncContext *func_ctx,
                          LLVMTypeRef vec_type,
                          const char *name);

bool
simd_bitcast_and_push_v128(const AOTCompContext *comp_ctx,
                           const AOTFuncContext *func_ctx,
                           LLVMValueRef vector,
                           const char *name);

#endif /* _SIMD_COMMON_H_ */