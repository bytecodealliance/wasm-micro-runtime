/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_bit_shifts.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../../aot/aot_runtime.h"

static bool
simd_shift(AOTCompContext *comp_ctx,
           AOTFuncContext *func_ctx,
           IntShift shift_op,
           LLVMTypeRef vector_type,
           LLVMTypeRef element_type,
           unsigned lane_width)
{
    LLVMValueRef vector, offset, width, undef, zeros, result;
    LLVMTypeRef zeros_type;

    POP_I32(offset);

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "vec"))) {
        goto fail;
    }

    if (!(width = LLVMConstInt(I32_TYPE, lane_width, true))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(offset =
            LLVMBuildURem(comp_ctx->builder, offset, width, "remainder"))) {
        HANDLE_FAILURE("LLVMBuildURem");
        goto fail;
    }

    if (I64_TYPE == element_type) {
        if (!(offset = LLVMBuildZExt(comp_ctx->builder, offset, element_type,
                                     "offset_scalar"))) {
            HANDLE_FAILURE("LLVMBuildZExt");
            goto fail;
        }
    }
    else {
        if (!(offset = LLVMBuildTruncOrBitCast(
                comp_ctx->builder, offset, element_type, "offset_scalar"))) {
            HANDLE_FAILURE("LLVMBuildTrunc");
            goto fail;
        }
    }

    /* create a vector with offset */
    if (!(undef = LLVMGetUndef(vector_type))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    if (!(zeros_type = LLVMVectorType(I32_TYPE, 128 / lane_width))) {
        HANDLE_FAILURE("LVMVectorType");
        goto fail;
    }

    if (!(zeros = LLVMConstNull(zeros_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    if (!(offset = LLVMBuildInsertElement(comp_ctx->builder, undef, offset,
                                          I32_ZERO, "base_vector"))) {
        HANDLE_FAILURE("LLVMBuildInsertElement");
        goto fail;
    }

    if (!(offset = LLVMBuildShuffleVector(comp_ctx->builder, offset, undef,
                                          zeros, "offset_vector"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    switch (shift_op) {
        case INT_SHL:
        {
            if (!(result =
                    LLVMBuildShl(comp_ctx->builder, vector, offset, "shl"))) {
                HANDLE_FAILURE("LLVMBuildShl");
                goto fail;
            }
            break;
        }
        case INT_SHR_S:
        {
            if (!(result = LLVMBuildAShr(comp_ctx->builder, vector, offset,
                                         "ashr"))) {
                HANDLE_FAILURE("LLVMBuildAShr");
                goto fail;
            }
            break;
        }
        case INT_SHR_U:
        {
            if (!(result = LLVMBuildLShr(comp_ctx->builder, vector, offset,
                                         "lshr"))) {
                HANDLE_FAILURE("LLVMBuildLShr");
                goto fail;
            }
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "result"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}

bool
aot_compile_simd_i8x16_shift(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             IntShift shift_op)
{
    return simd_shift(comp_ctx, func_ctx, shift_op, V128_i8x16_TYPE, INT8_TYPE,
                      8);
}

bool
aot_compile_simd_i16x8_shift(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             IntShift shift_op)
{
    return simd_shift(comp_ctx, func_ctx, shift_op, V128_i16x8_TYPE,
                      INT16_TYPE, 16);
}

bool
aot_compile_simd_i32x4_shift(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             IntShift shift_op)
{
    return simd_shift(comp_ctx, func_ctx, shift_op, V128_i32x4_TYPE, I32_TYPE,
                      32);
}

bool
aot_compile_simd_i64x2_shift(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             IntShift shift_op)
{
    return simd_shift(comp_ctx, func_ctx, shift_op, V128_i64x2_TYPE, I64_TYPE,
                      64);
}
