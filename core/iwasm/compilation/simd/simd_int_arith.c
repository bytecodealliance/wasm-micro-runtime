/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_int_arith.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../../aot/aot_runtime.h"

static bool
simd_v128_integer_arith(AOTCompContext *comp_ctx,
                        AOTFuncContext *func_ctx,
                        V128Arithmetic arith_op,
                        LLVMValueRef lhs,
                        LLVMValueRef rhs)
{
    LLVMValueRef result;

    switch (arith_op) {
        case V128_ADD:
            if (!(result = LLVMBuildAdd(comp_ctx->builder, lhs, rhs, "sum"))) {
                HANDLE_FAILURE("LLVMBuildAdd");
                goto fail;
            }
            break;
        case V128_SUB:
            if (!(result =
                    LLVMBuildSub(comp_ctx->builder, lhs, rhs, "difference"))) {
                HANDLE_FAILURE("LLVMBuildSub");
                goto fail;
            }
            break;
        case V128_MUL:
            if (!(result =
                    LLVMBuildMul(comp_ctx->builder, lhs, rhs, "product"))) {
                HANDLE_FAILURE("LLVMBuildMul");
                goto fail;
            }
            break;
        case V128_NEG:
            if (!(result = LLVMBuildNeg(comp_ctx->builder, lhs, "neg"))) {
                HANDLE_FAILURE("LLVMBuildNeg");
                goto fail;
            }
            break;
        default:
            result = NULL;
            bh_assert(0);
            break;
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    /* push result into the stack */
    PUSH_V128(result);

    return true;
fail:
    return false;
}

bool
aot_compile_simd_i8x16_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             V128Arithmetic arith_op)
{
    LLVMValueRef lhs, rhs;

    if (!(rhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i8x16_TYPE,
                                          "rhs"))) {
        goto fail;
    }

    if (!(lhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i8x16_TYPE,
                                          "lhs"))) {
        goto fail;
    }

    return simd_v128_integer_arith(comp_ctx, func_ctx, arith_op, lhs, rhs);
fail:
    return false;
}

bool
aot_compile_simd_i16x8_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             V128Arithmetic arith_op)
{
    LLVMValueRef lhs, rhs;

    if (!(rhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i16x8_TYPE,
                                          "rhs"))) {
        goto fail;
    }

    if (!(lhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i16x8_TYPE,
                                          "lhs"))) {
        goto fail;
    }

    return simd_v128_integer_arith(comp_ctx, func_ctx, arith_op, lhs, rhs);
fail:
    return false;
}

bool
aot_compile_simd_i32x4_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             V128Arithmetic arith_op)
{
    LLVMValueRef lhs, rhs;

    if (!(rhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i32x4_TYPE,
                                          "rhs"))) {
        goto fail;
    }

    if (!(lhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i32x4_TYPE,
                                          "lhs"))) {
        goto fail;
    }

    return simd_v128_integer_arith(comp_ctx, func_ctx, arith_op, lhs, rhs);
fail:
    return false;
}

bool
aot_compile_simd_i64x2_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             V128Arithmetic arith_op)
{
    LLVMValueRef lhs, rhs;

    POP_V128(rhs);
    POP_V128(lhs);

    return simd_v128_integer_arith(comp_ctx, func_ctx, arith_op, lhs, rhs);
fail:
    return false;
}

bool
aot_compile_simd_i8x16_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef number;

    if (!(number = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i8x16_TYPE, "number"))) {
        goto fail;
    }

    return simd_v128_integer_arith(comp_ctx, func_ctx, V128_NEG, number, NULL);

fail:
    return false;
}

bool
aot_compile_simd_i16x8_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef number;

    if (!(number = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i16x8_TYPE, "number"))) {
        goto fail;
    }

    return simd_v128_integer_arith(comp_ctx, func_ctx, V128_NEG, number, NULL);

fail:
    return false;
}

bool
aot_compile_simd_i32x4_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef number;

    if (!(number = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i32x4_TYPE, "number"))) {
        goto fail;
    }

    return simd_v128_integer_arith(comp_ctx, func_ctx, V128_NEG, number, NULL);

fail:
    return false;
}

bool
aot_compile_simd_i64x2_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef number;

    POP_V128(number);

    return simd_v128_integer_arith(comp_ctx, func_ctx, V128_NEG, number, NULL);

fail:
    return false;
}
