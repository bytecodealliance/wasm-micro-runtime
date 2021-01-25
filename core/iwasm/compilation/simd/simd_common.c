/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_common.h"

LLVMValueRef
simd_pop_v128_and_bitcast(const AOTCompContext *comp_ctx,
                          const AOTFuncContext *func_ctx,
                          LLVMTypeRef vec_type,
                          const char *name)
{
    LLVMValueRef number;

    POP_V128(number);

    if (!(number =
            LLVMBuildBitCast(comp_ctx->builder, number, vec_type, name))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    return number;
fail:
    return NULL;
}

bool
simd_bitcast_and_push_v128(const AOTCompContext *comp_ctx,
                           const AOTFuncContext *func_ctx,
                           LLVMValueRef vector,
                           const char *name)
{
    if (!(vector = LLVMBuildBitCast(comp_ctx->builder, vector, V128_i64x2_TYPE,
                                    name))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    /* push result into the stack */
    PUSH_V128(vector);

    return true;
fail:
    return false;
}