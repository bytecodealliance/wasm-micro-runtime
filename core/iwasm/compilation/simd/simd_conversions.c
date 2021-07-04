/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_conversions.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../aot_emit_numberic.h"
#include "../../aot/aot_runtime.h"

static bool
is_target_x86(AOTCompContext *comp_ctx)
{
    return !strncmp(comp_ctx->target_arch, "x86_64", 6) ||
           !strncmp(comp_ctx->target_arch, "i386", 4);
}

static bool
simd_integer_narrow(AOTCompContext *comp_ctx,
                    AOTFuncContext *func_ctx,
                    bool is_signed,
                    LLVMTypeRef in_vector_type,
                    LLVMTypeRef out_vector_type,
                    const char *instrinsic)
{
    LLVMValueRef vector1, vector2, result;
    LLVMTypeRef param_types[2] = { in_vector_type, in_vector_type };

    if (!(vector2 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                              in_vector_type, "vec2"))) {
        goto fail;
    }

    if (!(vector1 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                              in_vector_type, "vec1"))) {
        goto fail;
    }

    if (!(result =
            aot_call_llvm_intrinsic(comp_ctx, instrinsic, out_vector_type,
                                    param_types, 2, vector1, vector2))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}

static LLVMValueRef
build_intx4_vector(const AOTCompContext *comp_ctx,
                    const LLVMTypeRef element_type,
                    const int *element_value)
{
    LLVMValueRef vector, elements[4];
    unsigned i;

    for (i = 0; i < 4; i++) {
        if (!(elements[i] =
                LLVMConstInt(element_type, element_value[i], true))) {
            HANDLE_FAILURE("LLVMConstInst");
            goto fail;
        }
    }

    if (!(vector = LLVMConstVector(elements, 4))) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }
    return vector;
fail:
    return NULL;
}

static LLVMValueRef
build_intx8_vector(const AOTCompContext *comp_ctx,
                    const LLVMTypeRef element_type,
                    const int *element_value)
{
    LLVMValueRef vector, elements[8];
    unsigned i;

    for (i = 0; i < 8; i++) {
        if (!(elements[i] =
                LLVMConstInt(element_type, element_value[i], true))) {
            HANDLE_FAILURE("LLVMConstInst");
            goto fail;
        }
    }

    if (!(vector = LLVMConstVector(elements, 8))) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }

    return vector;
fail:
    return NULL;
}

static LLVMValueRef
build_intx16_vector(const AOTCompContext *comp_ctx,
                    const LLVMTypeRef element_type,
                    const int *element_value)
{
    LLVMValueRef vector, elements[16];
    unsigned i;

    for (i = 0; i < 16; i++) {
        if (!(elements[i] =
                LLVMConstInt(element_type, element_value[i], true))) {
            HANDLE_FAILURE("LLVMConstInst");
            goto fail;
        }
    }

    if (!(vector = LLVMConstVector(elements, 16))) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }

    return vector;
fail:
    return NULL;
}

bool
aot_compile_simd_i8x16_narrow_i16x8_x86(AOTCompContext *comp_ctx,
                                    AOTFuncContext *func_ctx,
                                    bool is_signed)
{
    return simd_integer_narrow(
      comp_ctx, func_ctx, is_signed, V128_i16x8_TYPE, V128_i8x16_TYPE,
      is_signed ? "llvm.x86.sse2.packsswb.128" : "llvm.x86.sse2.packuswb.128");
}

bool
aot_compile_simd_i16x8_narrow_i32x4_x86(AOTCompContext *comp_ctx,
                                    AOTFuncContext *func_ctx,
                                    bool is_signed)
{
    return simd_integer_narrow(
      comp_ctx, func_ctx, is_signed, V128_i32x4_TYPE, V128_i16x8_TYPE,
      is_signed ? "llvm.x86.sse2.packssdw.128" : "llvm.x86.sse41.packusdw");
}

static bool
aot_compile_simd_i8x16_narrow_i16x8_common(AOTCompContext *comp_ctx,
                                           AOTFuncContext *func_ctx,
                                           bool is_signed)
{
    LLVMValueRef vector1, vector2, result, vector_min, vector_max, shuffle,
      vector1_clamped, vector2_clamped, vector1_trunced, vector2_trunced,
      shuffle_vector;
    LLVMValueRef v1_gt_max, v1_lt_min, v2_gt_max, v2_lt_min;

    int min_s_array[8] = { 0xff80, 0xff80, 0xff80, 0xff80,
                           0xff80, 0xff80, 0xff80, 0xff80 };
    int max_s_array[8] = { 0x007f, 0x007f, 0x007f, 0x007f,
                           0x007f, 0x007f, 0x007f, 0x007f };

    int min_u_array[8] = { 0x0000, 0x0000, 0x0000, 0x0000,
                           0x0000, 0x0000, 0x0000, 0x0000 };
    int max_u_array[8] = { 0x00ff, 0x00ff, 0x00ff, 0x00ff,
                           0x00ff, 0x00ff, 0x00ff, 0x00ff };

    int shuffle_array[16] = { 0, 1, 2,  3,  4,  5,  6,  7,
                              8, 9, 10, 11, 12, 13, 14, 15 };

    if (!(vector2 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                              V128_i16x8_TYPE, "vec2"))) {
        goto fail;
    }

    if (!(vector1 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                              V128_i16x8_TYPE, "vec1"))) {
        goto fail;
    }

    if (!(vector_min = build_intx8_vector(
            comp_ctx, INT16_TYPE, is_signed ? min_s_array : min_u_array))) {
        goto fail;
    }
    if (!(vector_max = build_intx8_vector(
            comp_ctx, INT16_TYPE, is_signed ? max_s_array : max_u_array))) {
        goto fail;
    }
    if (!(shuffle = build_intx16_vector(comp_ctx, I32_TYPE, shuffle_array))) {
        goto fail;
    }

    if (!(v1_gt_max = LLVMBuildICmp(comp_ctx->builder, LLVMIntSGT, vector1,
                                    vector_max, "v1_great_than_max"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(v2_gt_max = LLVMBuildICmp(comp_ctx->builder, LLVMIntSGT, vector2,
                                    vector_max, "v2_great_than_max"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(v1_lt_min = LLVMBuildICmp(comp_ctx->builder, LLVMIntSLT, vector1,
                                    vector_min, "v1_less_than_min"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(v2_lt_min = LLVMBuildICmp(comp_ctx->builder, LLVMIntSLT, vector2,
                                    vector_min, "v2_less_than_min"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(vector1_clamped =
            LLVMBuildSelect(comp_ctx->builder, v1_gt_max, vector_max, vector1,
                            "vector1_clamped_max"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector1_clamped =
            LLVMBuildSelect(comp_ctx->builder, v1_lt_min, vector_min,
                            vector1_clamped, "vector1_clamped_min"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector2_clamped =
            LLVMBuildSelect(comp_ctx->builder, v2_gt_max, vector_max, vector2,
                            "vector2_clamped_max"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector2_clamped =
            LLVMBuildSelect(comp_ctx->builder, v2_lt_min, vector_min,
                            vector2_clamped, "vector2_clamped_min"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector1_trunced =
            LLVMBuildTrunc(comp_ctx->builder, vector1_clamped,
                           LLVMVectorType(INT8_TYPE, 8), "vector1_trunced"))) {
        HANDLE_FAILURE("LLVMBuildTrunc");
        goto fail;
    }

    if (!(vector2_trunced =
            LLVMBuildTrunc(comp_ctx->builder, vector2_clamped,
                           LLVMVectorType(INT8_TYPE, 8), "vector2_trunced"))) {
        HANDLE_FAILURE("LLVMBuildTrunc");
        goto fail;
    }

    if (!(shuffle_vector = LLVMBuildShuffleVector(
            comp_ctx->builder, vector1_trunced, vector2_trunced, shuffle,
            "shuffle_vector"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, shuffle_vector,
                                    V128_i64x2_TYPE, "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;

fail:
    return false;
}

bool
aot_compile_simd_i8x16_narrow_i16x8(AOTCompContext *comp_ctx,
                                    AOTFuncContext *func_ctx,
                                    bool is_signed)
{
    if (is_target_x86(comp_ctx)) {
        return aot_compile_simd_i8x16_narrow_i16x8_x86(comp_ctx, func_ctx,
                                                       is_signed);
    }
    else {
        return aot_compile_simd_i8x16_narrow_i16x8_common(comp_ctx, func_ctx,
                                                          is_signed);
    }
}

static bool
aot_compile_simd_i16x8_narrow_i32x4_common(AOTCompContext *comp_ctx,
                                           AOTFuncContext *func_ctx,
                                           bool is_signed)
{
    LLVMValueRef vector1, vector2, result, vector_min, vector_max, shuffle,
      vector1_clamped, vector2_clamped, vector1_trunced, vector2_trunced,
      shuffle_vector;
    LLVMValueRef v1_gt_max, v1_lt_min, v2_gt_max, v2_lt_min;

    int min_s_array[4] = { 0xffff8000, 0xffff8000, 0xffff8000, 0xffff8000 };
    int32 max_s_array[4] = { 0x00007fff, 0x00007fff, 0x00007fff, 0x00007fff };

    int min_u_array[4] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
    int max_u_array[4] = { 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };

    int shuffle_array[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    if (!(vector2 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                              V128_i32x4_TYPE, "vec2"))) {
        goto fail;
    }

    if (!(vector1 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                              V128_i32x4_TYPE, "vec1"))) {
        goto fail;
    }

    if (!(vector_min = build_intx4_vector(
            comp_ctx, I32_TYPE, is_signed ? min_s_array : min_u_array))) {
        goto fail;
    }
    if (!(vector_max = build_intx4_vector(
            comp_ctx, I32_TYPE, is_signed ? max_s_array : max_u_array))) {
        goto fail;
    }
    if (!(shuffle = build_intx8_vector(comp_ctx, I32_TYPE, shuffle_array))) {
        goto fail;
    }

    if (!(v1_gt_max = LLVMBuildICmp(comp_ctx->builder, LLVMIntSGT, vector1,
                                    vector_max, "v1_great_than_max"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(v2_gt_max = LLVMBuildICmp(comp_ctx->builder, LLVMIntSGT, vector2,
                                    vector_max, "v2_great_than_max"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(v1_lt_min = LLVMBuildICmp(comp_ctx->builder, LLVMIntSLT, vector1,
                                    vector_min, "v1_less_than_min"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(v2_lt_min = LLVMBuildICmp(comp_ctx->builder, LLVMIntSLT, vector2,
                                    vector_min, "v2_less_than_min"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    if (!(vector1_clamped =
            LLVMBuildSelect(comp_ctx->builder, v1_gt_max, vector_max, vector1,
                            "vector1_clamped_max"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector1_clamped =
            LLVMBuildSelect(comp_ctx->builder, v1_lt_min, vector_min,
                            vector1_clamped, "vector1_clamped_min"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector2_clamped =
            LLVMBuildSelect(comp_ctx->builder, v2_gt_max, vector_max, vector2,
                            "vector2_clamped_max"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector2_clamped =
            LLVMBuildSelect(comp_ctx->builder, v2_lt_min, vector_min,
                            vector2_clamped, "vector2_clamped_min"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(vector1_trunced = LLVMBuildTrunc(comp_ctx->builder, vector1_clamped,
                                           LLVMVectorType(INT16_TYPE, 4),
                                           "vector1_trunced"))) {
        HANDLE_FAILURE("LLVMBuildTrunc");
        goto fail;
    }

    if (!(vector2_trunced = LLVMBuildTrunc(comp_ctx->builder, vector2_clamped,
                                           LLVMVectorType(INT16_TYPE, 4),
                                           "vector2_trunced"))) {
        HANDLE_FAILURE("LLVMBuildTrunc");
        goto fail;
    }

    if (!(shuffle_vector = LLVMBuildShuffleVector(
            comp_ctx->builder, vector1_trunced, vector2_trunced, shuffle,
            "shuffle_vector"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, shuffle_vector,
                                    V128_i64x2_TYPE, "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;

fail:
    return false;
}

bool
aot_compile_simd_i16x8_narrow_i32x4(AOTCompContext *comp_ctx,
                                    AOTFuncContext *func_ctx,
                                    bool is_signed)
{
    if (is_target_x86(comp_ctx)) {
        return aot_compile_simd_i16x8_narrow_i32x4_x86(comp_ctx, func_ctx,
                                                       is_signed);
    }
    else {
        return aot_compile_simd_i16x8_narrow_i32x4_common(comp_ctx, func_ctx,
                                                          is_signed);
    }
}

bool
aot_compile_simd_i16x8_widen_i8x16(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx,
                                   bool is_low_half,
                                   bool is_signed)
{
    LLVMValueRef vector, undef, mask_high[8], mask_low[8], mask, shuffled,
      result;
    uint8 mask_high_value[8] = { 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf },
          mask_low_value[8] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 }, i;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i8x16_TYPE, "vec"))) {
        goto fail;
    }

    if (!(undef = LLVMGetUndef(V128_i8x16_TYPE))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    /* create a mask */
    for (i = 0; i < 8; i++) {
        mask_high[i] = LLVMConstInt(I32_TYPE, mask_high_value[i], true);
        mask_low[i] = LLVMConstInt(I32_TYPE, mask_low_value[i], true);
    }

    mask = is_low_half ? LLVMConstVector(mask_low, 8)
                       : LLVMConstVector(mask_high, 8);
    if (!mask) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }

    /* retrive the low or high half */
    if (!(shuffled = LLVMBuildShuffleVector(comp_ctx->builder, vector, undef,
                                            mask, "shuffled"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    if (is_signed) {
        if (!(result = LLVMBuildSExt(comp_ctx->builder, shuffled,
                                     V128_i16x8_TYPE, "ext"))) {
            HANDLE_FAILURE("LLVMBuildSExt");
            goto fail;
        }
    }
    else {
        if (!(result = LLVMBuildZExt(comp_ctx->builder, shuffled,
                                     V128_i16x8_TYPE, "ext"))) {
            HANDLE_FAILURE("LLVMBuildZExt");
            goto fail;
        }
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}

bool
aot_compile_simd_i32x4_widen_i16x8(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx,
                                   bool is_low_half,
                                   bool is_signed)
{
    LLVMValueRef vector, undef, mask_high[4], mask_low[4], mask, shuffled,
      result;
    uint8 mask_high_value[4] = { 0x4, 0x5, 0x6, 0x7 },
          mask_low_value[4] = { 0x0, 0x1, 0x2, 0x3 }, i;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i16x8_TYPE, "vec"))) {
        goto fail;
    }

    if (!(undef = LLVMGetUndef(V128_i16x8_TYPE))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    /* create a mask */
    for (i = 0; i < 4; i++) {
        mask_high[i] = LLVMConstInt(I32_TYPE, mask_high_value[i], true);
        mask_low[i] = LLVMConstInt(I32_TYPE, mask_low_value[i], true);
    }

    mask = is_low_half ? LLVMConstVector(mask_low, 4)
                       : LLVMConstVector(mask_high, 4);
    if (!mask) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }

    /* retrive the low or high half */
    if (!(shuffled = LLVMBuildShuffleVector(comp_ctx->builder, vector, undef,
                                            mask, "shuffled"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    if (is_signed) {
        if (!(result = LLVMBuildSExt(comp_ctx->builder, shuffled,
                                     V128_i32x4_TYPE, "ext"))) {
            HANDLE_FAILURE("LLVMBuildSExt");
            goto fail;
        }
    }
    else {
        if (!(result = LLVMBuildZExt(comp_ctx->builder, shuffled,
                                     V128_i32x4_TYPE, "ext"))) {
            HANDLE_FAILURE("LLVMBuildZExt");
            goto fail;
        }
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}

static LLVMValueRef
simd_build_const_f32x4(AOTCompContext *comp_ctx,
                       AOTFuncContext *func_ctx,
                       float f)
{
    LLVMValueRef elements[4], vector;

    if (!(elements[0] = LLVMConstReal(F32_TYPE, f))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    elements[1] = elements[2] = elements[3] = elements[0];

    if (!(vector = LLVMConstVector(elements, 4))) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }

    return vector;
fail:
    return NULL;
}

static LLVMValueRef
simd_build_const_i32x4(AOTCompContext *comp_ctx,
                       AOTFuncContext *func_ctx,
                       uint64 integer,
                       bool is_signed)
{
    LLVMValueRef elements[4], vector;

    if (!(elements[0] = LLVMConstInt(I32_TYPE, integer, is_signed))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    elements[1] = elements[2] = elements[3] = elements[0];

    if (!(vector = LLVMConstVector(elements, 4))) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }

    return vector;
fail:
    return NULL;
}

bool
aot_compile_simd_i32x4_trunc_sat_f32x4(AOTCompContext *comp_ctx,
                                       AOTFuncContext *func_ctx,
                                       bool is_signed)
{
    LLVMValueRef vector, zeros, is_nan, max_float_v, min_float_v, is_ge_max,
      is_le_min, result, max_int_v, min_int_v;
    uint32 max_ui = 0xFFffFFff, min_ui = 0x0;
    int32 max_si = 0x7FFFffff, min_si = 0x80000000;
    float max_f_ui = 4294967296.0f, min_f_ui = 0.0f, max_f_si = 2147483647.0f,
          min_f_si = -2147483648.0f;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_f32x4_TYPE, "vec"))) {
        goto fail;
    }

    if (!(zeros = LLVMConstNull(V128_f32x4_TYPE))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    if (is_signed) {
        if (!(max_float_v =
                simd_build_const_f32x4(comp_ctx, func_ctx, max_f_si))) {
            goto fail;
        }

        if (!(min_float_v =
                simd_build_const_f32x4(comp_ctx, func_ctx, min_f_si))) {
            goto fail;
        }

        if (!(max_int_v =
                simd_build_const_i32x4(comp_ctx, func_ctx, max_si, true))) {
            goto fail;
        }

        if (!(min_int_v =
                simd_build_const_i32x4(comp_ctx, func_ctx, min_si, true))) {
            goto fail;
        }
    }
    else {
        if (!(max_float_v =
                simd_build_const_f32x4(comp_ctx, func_ctx, max_f_ui))) {
            goto fail;
        }

        if (!(min_float_v =
                simd_build_const_f32x4(comp_ctx, func_ctx, min_f_ui))) {
            goto fail;
        }

        if (!(max_int_v =
                simd_build_const_i32x4(comp_ctx, func_ctx, max_ui, false))) {
            goto fail;
        }

        if (!(min_int_v =
                simd_build_const_i32x4(comp_ctx, func_ctx, min_ui, false))) {
            goto fail;
        }
    }

    if (!(is_nan = LLVMBuildFCmp(comp_ctx->builder, LLVMRealORD, vector, zeros,
                                 "is_nan"))) {
        HANDLE_FAILURE("LLVMBuildFCmp");
        goto fail;
    }

    if (!(is_le_min = LLVMBuildFCmp(comp_ctx->builder, LLVMRealOLE, vector,
                                    min_float_v, "le_min"))) {
        HANDLE_FAILURE("LLVMBuildFCmp");
        goto fail;
    }

    if (!(is_ge_max = LLVMBuildFCmp(comp_ctx->builder, LLVMRealOGE, vector,
                                    max_float_v, "ge_max"))) {
        HANDLE_FAILURE("LLVMBuildFCmp");
        goto fail;
    }

    if (is_signed) {
        if (!(result = LLVMBuildFPToSI(comp_ctx->builder, vector,
                                       V128_i32x4_TYPE, "truncated"))) {
            HANDLE_FAILURE("LLVMBuildSIToFP");
            goto fail;
        }
    }
    else {
        if (!(result = LLVMBuildFPToUI(comp_ctx->builder, vector,
                                       V128_i32x4_TYPE, "truncated"))) {
            HANDLE_FAILURE("LLVMBuildUIToFP");
            goto fail;
        }
    }

    if (!(result = LLVMBuildSelect(comp_ctx->builder, is_ge_max, max_int_v,
                                   result, "sat_w_max"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(result = LLVMBuildSelect(comp_ctx->builder, is_le_min, min_int_v,
                                   result, "sat_w_min"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(result = LLVMBuildSelect(comp_ctx->builder, is_nan, result,
                                   V128_i32x4_ZERO, "sat_w_nan"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}

bool
aot_compile_simd_f32x4_convert_i32x4(AOTCompContext *comp_ctx,
                                     AOTFuncContext *func_ctx,
                                     bool is_signed)
{
    LLVMValueRef vector, result;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i32x4_TYPE, "vec"))) {
        goto fail;
    }

    if (is_signed) {
        if (!(result = LLVMBuildSIToFP(comp_ctx->builder, vector,
                                       V128_f32x4_TYPE, "converted"))) {
            HANDLE_FAILURE("LLVMBuildSIToFP");
            goto fail;
        }
    }
    else {
        if (!(result = LLVMBuildUIToFP(comp_ctx->builder, vector,
                                       V128_f32x4_TYPE, "converted"))) {
            HANDLE_FAILURE("LLVMBuildSIToFP");
            goto fail;
        }
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "ret"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}
