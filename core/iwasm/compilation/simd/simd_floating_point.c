/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_floating_point.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../aot_emit_numberic.h"
#include "../../aot/aot_runtime.h"

static LLVMValueRef
simd_v128_float_cmp(AOTCompContext *comp_ctx,
                    AOTFuncContext *func_ctx,
                    FloatArithmetic arith_op,
                    LLVMValueRef lhs,
                    LLVMValueRef rhs)
{
    LLVMValueRef result;
    LLVMRealPredicate op;

    op = FLOAT_MIN == arith_op ? LLVMRealULT : LLVMRealUGT;

    if (!(result = LLVMBuildFCmp(comp_ctx->builder, op, lhs, rhs, "cmp"))) {
        HANDLE_FAILURE("LLVMBuildFCmp");
        goto fail;
    }

    if (!(result =
            LLVMBuildSelect(comp_ctx->builder, result, lhs, rhs, "select"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    return result;
fail:
    return NULL;
}

static bool
simd_v128_float_arith(AOTCompContext *comp_ctx,
                      AOTFuncContext *func_ctx,
                      FloatArithmetic arith_op,
                      LLVMTypeRef vector_type)
{
    LLVMValueRef lhs, rhs, result;

    if (!(rhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "rhs"))) {
        goto fail;
    }

    if (!(lhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "lhs"))) {
        goto fail;
    }

    switch (arith_op) {
        case FLOAT_ADD:
            if (!(result =
                    LLVMBuildFAdd(comp_ctx->builder, lhs, rhs, "sum"))) {
                HANDLE_FAILURE("LLVMBuildFAdd");
                goto fail;
            }
            break;
        case FLOAT_SUB:
            if (!(result = LLVMBuildFSub(comp_ctx->builder, lhs, rhs,
                                         "difference"))) {
                HANDLE_FAILURE("LLVMBuildFSub");
                goto fail;
            }
            break;
        case FLOAT_MUL:
            if (!(result =
                    LLVMBuildFMul(comp_ctx->builder, lhs, rhs, "product"))) {
                HANDLE_FAILURE("LLVMBuildFMul");
                goto fail;
            }
            break;
        case FLOAT_DIV:
            if (!(result =
                    LLVMBuildFDiv(comp_ctx->builder, lhs, rhs, "quotient"))) {
                HANDLE_FAILURE("LLVMBuildFDiv");
                goto fail;
            }
            break;
        case FLOAT_MIN:
            if (!(result = simd_v128_float_cmp(comp_ctx, func_ctx, FLOAT_MIN,
                                               lhs, rhs))) {
                goto fail;
            }
            break;
        case FLOAT_MAX:
            if (!(result = simd_v128_float_cmp(comp_ctx, func_ctx, FLOAT_MAX,
                                               lhs, rhs))) {
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
aot_compile_simd_f32x4_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             FloatArithmetic arith_op)
{
    return simd_v128_float_arith(comp_ctx, func_ctx, arith_op,
                                 V128_f32x4_TYPE);
}

bool
aot_compile_simd_f64x2_arith(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             FloatArithmetic arith_op)
{
    return simd_v128_float_arith(comp_ctx, func_ctx, arith_op,
                                 V128_f64x2_TYPE);
}

static bool
simd_v128_float_neg(AOTCompContext *comp_ctx,
                    AOTFuncContext *func_ctx,
                    LLVMTypeRef vector_type)
{
    LLVMValueRef number, result;

    if (!(number = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "number"))) {
        goto fail;
    }

    if (!(result = LLVMBuildFNeg(comp_ctx->builder, number, "neg"))) {
        HANDLE_FAILURE("LLVMBuildFNeg");
        goto fail;
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
aot_compile_simd_f32x4_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_neg(comp_ctx, func_ctx, V128_f32x4_TYPE);
}

bool
aot_compile_simd_f64x2_neg(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_neg(comp_ctx, func_ctx, V128_f64x2_TYPE);
}

static bool
simd_v128_float_intrinsic(AOTCompContext *comp_ctx,
                          AOTFuncContext *func_ctx,
                          LLVMTypeRef vector_type,
                          const char *intrinsic)
{
    LLVMValueRef number, result;
    LLVMTypeRef param_types[1] = { vector_type };

    if (!(number = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "number"))) {
        goto fail;
    }

    if (!(result = aot_call_llvm_intrinsic(comp_ctx, func_ctx, intrinsic, vector_type,
                                           param_types, 1, number))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
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
aot_compile_simd_f32x4_abs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f32x4_TYPE,
                                     "llvm.fabs.v4f32");
}

bool
aot_compile_simd_f64x2_abs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f64x2_TYPE,
                                     "llvm.fabs.v2f64");
}

bool
aot_compile_simd_f32x4_sqrt(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f32x4_TYPE,
                                     "llvm.sqrt.v4f32");
}

bool
aot_compile_simd_f64x2_sqrt(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f64x2_TYPE,
                                     "llvm.sqrt.v2f64");
}

bool
aot_compile_simd_f32x4_ceil(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f32x4_TYPE,
                                     "llvm.ceil.v4f32");
}

bool
aot_compile_simd_f64x2_ceil(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f64x2_TYPE,
                                     "llvm.ceil.v2f64");
}

bool
aot_compile_simd_f32x4_floor(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f32x4_TYPE,
                                     "llvm.floor.v4f32");
}

bool
aot_compile_simd_f64x2_floor(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f64x2_TYPE,
                                     "llvm.floor.v2f64");
}

bool
aot_compile_simd_f32x4_trunc(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f32x4_TYPE,
                                     "llvm.trunc.v4f32");
}

bool
aot_compile_simd_f64x2_trunc(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f64x2_TYPE,
                                     "llvm.trunc.v2f64");
}

bool
aot_compile_simd_f32x4_nearest(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f32x4_TYPE,
                                     "llvm.rint.v4f32");
}

bool
aot_compile_simd_f64x2_nearest(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_float_intrinsic(comp_ctx, func_ctx, V128_f64x2_TYPE,
                                     "llvm.rint.v2f64");
}
