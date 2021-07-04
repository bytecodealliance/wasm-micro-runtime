/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_sat_int_arith.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../../aot/aot_runtime.h"

static bool
simd_v128_integer_arith(AOTCompContext *comp_ctx,
                        AOTFuncContext *func_ctx,
                        LLVMTypeRef vector_type,
                        char *intrinsics_s_u[2],
                        bool is_signed)
{
    LLVMValueRef lhs, rhs, result;
    LLVMTypeRef param_types[2];

    if (!(rhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "rhs"))) {
        goto fail;
    }

    if (!(lhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "lhs"))) {
        goto fail;
    }

    param_types[0] = vector_type;
    param_types[1] = vector_type;

    if (!(result = aot_call_llvm_intrinsic(
            comp_ctx, is_signed ? intrinsics_s_u[0] : intrinsics_s_u[1],
            vector_type, param_types, 2, lhs, rhs))) {
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
aot_compile_simd_i8x16_saturate(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx,
                                V128Arithmetic arith_op,
                                bool is_signed)
{
    char *intrinsics[2] = { 0 };
    bool result = false;
    switch (arith_op) {
        case V128_ADD:
            intrinsics[0] = "llvm.sadd.sat.v16i8";
            intrinsics[1] = "llvm.uadd.sat.v16i8";
            result = simd_v128_integer_arith(
              comp_ctx, func_ctx, V128_i8x16_TYPE, intrinsics, is_signed);
            break;
        case V128_SUB:
            intrinsics[0] = "llvm.ssub.sat.v16i8";
            intrinsics[1] = "llvm.usub.sat.v16i8";
            result = simd_v128_integer_arith(
              comp_ctx, func_ctx, V128_i8x16_TYPE, intrinsics, is_signed);
            break;
        default:
            bh_assert(0);
            break;
    }

    return result;
}

bool
aot_compile_simd_i16x8_saturate(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx,
                                V128Arithmetic arith_op,
                                bool is_signed)
{
    char *intrinsics[2] = { 0 };
    bool result = false;
    switch (arith_op) {
        case V128_ADD:
            intrinsics[0] = "llvm.sadd.sat.v8i16";
            intrinsics[1] = "llvm.uadd.sat.v8i16";
            result = simd_v128_integer_arith(
              comp_ctx, func_ctx, V128_i16x8_TYPE, intrinsics, is_signed);
            break;
        case V128_SUB:
            intrinsics[0] = "llvm.ssub.sat.v8i16";
            intrinsics[1] = "llvm.usub.sat.v8i16";
            result = simd_v128_integer_arith(
              comp_ctx, func_ctx, V128_i16x8_TYPE, intrinsics, is_signed);
            break;
        default:
            bh_assert(0);
            break;
    }

    return result;
}

static bool
simd_v128_cmp(AOTCompContext *comp_ctx,
              AOTFuncContext *func_ctx,
              LLVMTypeRef vector_type,
              V128Arithmetic arith_op,
              bool is_signed)
{
    LLVMValueRef lhs, rhs, result;
    LLVMIntPredicate op;

    if (!(rhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "rhs"))) {
        goto fail;
    }

    if (!(lhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "lhs"))) {
        goto fail;
    }

    if (V128_MIN == arith_op) {
        op = is_signed ? LLVMIntSLT : LLVMIntULT;
    }
    else {
        op = is_signed ? LLVMIntSGT : LLVMIntUGT;
    }

    if (!(result = LLVMBuildICmp(comp_ctx->builder, op, lhs, rhs, "cmp"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    if (!(result =
            LLVMBuildSelect(comp_ctx->builder, result, lhs, rhs, "select"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
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
aot_compile_simd_i8x16_cmp(AOTCompContext *comp_ctx,
                           AOTFuncContext *func_ctx,
                           V128Arithmetic arith_op,
                           bool is_signed)
{
    return simd_v128_cmp(comp_ctx, func_ctx, V128_i8x16_TYPE, arith_op,
                         is_signed);
}

bool
aot_compile_simd_i16x8_cmp(AOTCompContext *comp_ctx,
                           AOTFuncContext *func_ctx,
                           V128Arithmetic arith_op,
                           bool is_signed)
{
    return simd_v128_cmp(comp_ctx, func_ctx, V128_i16x8_TYPE, arith_op,
                         is_signed);
}

bool
aot_compile_simd_i32x4_cmp(AOTCompContext *comp_ctx,
                           AOTFuncContext *func_ctx,
                           V128Arithmetic arith_op,
                           bool is_signed)
{
    return simd_v128_cmp(comp_ctx, func_ctx, V128_i32x4_TYPE, arith_op,
                         is_signed);
}

static bool
simd_v128_abs(AOTCompContext *comp_ctx,
              AOTFuncContext *func_ctx,
              LLVMTypeRef vector_type)
{
    LLVMValueRef vector, negs, zeros, cond, result;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "vec"))) {
        goto fail;
    }

    if (!(negs = LLVMBuildNeg(comp_ctx->builder, vector, "neg"))) {
        HANDLE_FAILURE("LLVMBuildNeg");
        goto fail;
    }

    if (!(zeros = LLVMConstNull(vector_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    if (!(cond = LLVMBuildICmp(comp_ctx->builder, LLVMIntSGE, vector, zeros,
                               "ge_zero"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    if (!(result = LLVMBuildSelect(comp_ctx->builder, cond, vector, negs,
                                   "select"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
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
aot_compile_simd_i8x16_abs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_abs(comp_ctx, func_ctx, V128_i8x16_TYPE);
}

bool
aot_compile_simd_i16x8_abs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_abs(comp_ctx, func_ctx, V128_i16x8_TYPE);
}

bool
aot_compile_simd_i32x4_abs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return simd_v128_abs(comp_ctx, func_ctx, V128_i32x4_TYPE);
}

/* (v1 + v2 + 1) / 2 */
static bool
simd_v128_avg(AOTCompContext *comp_ctx,
              AOTFuncContext *func_ctx,
              LLVMTypeRef vector_type,
              LLVMTypeRef element_type,
              unsigned lane_width)
{
    LLVMValueRef lhs, rhs, undef, zeros, ones, result;
    LLVMTypeRef ext_type;

    if (!(rhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "rhs"))) {
        goto fail;
    }

    if (!(lhs = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                          "lhs"))) {
        goto fail;
    }

    if (!(ext_type = LLVMVectorType(I32_TYPE, lane_width))) {
        HANDLE_FAILURE("LLVMVectorType");
        goto fail;
    }

    if (!(lhs = LLVMBuildZExt(comp_ctx->builder, lhs, ext_type, "left_ext"))) {
        HANDLE_FAILURE("LLVMBuildZExt");
        goto fail;
    }

    if (!(rhs =
            LLVMBuildZExt(comp_ctx->builder, rhs, ext_type, "right_ext"))) {
        HANDLE_FAILURE("LLVMBuildZExt");
        goto fail;
    }

    if (!(undef = LLVMGetUndef(ext_type))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    if (!(zeros = LLVMConstNull(ext_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    if (!(ones = LLVMConstInt(I32_TYPE, 1, true))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(ones = LLVMBuildInsertElement(comp_ctx->builder, undef, ones,
                                        I32_ZERO, "base_ones"))) {
        HANDLE_FAILURE("LLVMBuildInsertElement");
        goto fail;
    }

    if (!(ones = LLVMBuildShuffleVector(comp_ctx->builder, ones, undef, zeros,
                                        "ones"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    if (!(result = LLVMBuildAdd(comp_ctx->builder, lhs, rhs, "a_add_b"))) {
        HANDLE_FAILURE("LLVMBuildAdd");
        goto fail;
    }

    if (!(result = LLVMBuildAdd(comp_ctx->builder, result, ones, "plus_1"))) {
        HANDLE_FAILURE("LLVMBuildAdd");
        goto fail;
    }

    if (!(result = LLVMBuildLShr(comp_ctx->builder, result, ones, "avg"))) {
        HANDLE_FAILURE("LLVMBuildLShr");
        goto fail;
    }

    if (!(result = LLVMBuildTrunc(comp_ctx->builder, result, vector_type,
                                  "avg_trunc"))) {
        HANDLE_FAILURE("LLVMBuildTrunc");
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
aot_compile_simd_i8x16_avgr_u(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx)
{
    return simd_v128_avg(comp_ctx, func_ctx, V128_i8x16_TYPE, INT8_TYPE, 16);
}

bool
aot_compile_simd_i16x8_avgr_u(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx)
{
    return simd_v128_avg(comp_ctx, func_ctx, V128_i16x8_TYPE, INT16_TYPE, 8);
}