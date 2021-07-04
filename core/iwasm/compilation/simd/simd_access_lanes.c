/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_access_lanes.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../../aot/aot_runtime.h"

static bool
is_target_x86(AOTCompContext *comp_ctx)
{
    return !strncmp(comp_ctx->target_arch, "x86_64", 6) ||
           !strncmp(comp_ctx->target_arch, "i386", 4);
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
aot_compile_simd_shuffle(AOTCompContext *comp_ctx,
                         AOTFuncContext *func_ctx,
                         const uint8 *frame_ip)
{
    LLVMValueRef vec1, vec2, mask, result;
    uint8 imm[16] = { 0 };
    int values[16];
    unsigned i;

    wasm_runtime_read_v128(frame_ip, (uint64 *)imm, (uint64 *)(imm + 8));
    for (i = 0; i < 16; i++) {
        values[i] = imm[i];
    }

    if (!(vec2 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i8x16_TYPE,
                                           "vec2"))) {
        goto fail;
    }

    if (!(vec1 = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i8x16_TYPE,
                                           "vec1"))) {
        goto fail;
    }

    /* build a vector <16 x i32> */
    if (!(mask = build_intx16_vector(comp_ctx, I32_TYPE, values))) {
        goto fail;
    }

    if (!(result = LLVMBuildShuffleVector(comp_ctx->builder, vec1, vec2, mask,
                                          "new_vector"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
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

/* shufflevector is not an option, since it requires *mask as a const */
bool
aot_compile_simd_swizzle_x86(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef vector, mask, max_lanes, condition, mask_lanes, result;
    LLVMTypeRef param_types[2];
    int max_lane_id[16] = { 16, 16, 16, 16, 16, 16, 16, 16,
                            16, 16, 16, 16, 16, 16, 16, 16 },
        mask_lane_id[16] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
                             0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };

    if (!(mask = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i8x16_TYPE,
                                           "mask"))) {
        goto fail;
    }

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i8x16_TYPE, "vec"))) {
        goto fail;
    }

    /* icmp uge <16 x i8> mask, <16, 16, 16, 16, ...> */
    if (!(max_lanes = build_intx16_vector(comp_ctx, INT8_TYPE, max_lane_id))) {
        goto fail;
    }

    if (!(condition = LLVMBuildICmp(comp_ctx->builder, LLVMIntUGE, mask,
                                    max_lanes, "compare_with_16"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    /*  if the highest bit of every i8 of mask is 1, means doesn't pick up from vector */
    /* select <16 x i1> %condition, <16 x i8> <0x80, 0x80, ...>, <16 x i8> %mask */
    if (!(mask_lanes =
            build_intx16_vector(comp_ctx, INT8_TYPE, mask_lane_id))) {
        goto fail;
    }

    if (!(mask = LLVMBuildSelect(comp_ctx->builder, condition, mask_lanes,
                                 mask, "mask"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    param_types[0] = V128_i8x16_TYPE;
    param_types[1] = V128_i8x16_TYPE;
    if (!(result = aot_call_llvm_intrinsic(
            comp_ctx, "llvm.x86.ssse3.pshuf.b.128", V128_i8x16_TYPE,
            param_types, 2, vector, mask))) {
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

static bool
aot_compile_simd_swizzle_common(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef vector, mask, default_lane_value, condition, max_lane_id,
      result, idx, id, replace_with_zero, elem, elem_or_zero, undef;
    uint8 i;

    int const_lane_ids[16] = { 16, 16, 16, 16, 16, 16, 16, 16,
                               16, 16, 16, 16, 16, 16, 16, 16 },
        const_zeors[16] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

    if (!(mask = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, V128_i8x16_TYPE,
                                           "mask"))) {
        goto fail;
    }

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx,
                                             V128_i8x16_TYPE, "vec"))) {
        goto fail;
    }

    if (!(undef = LLVMGetUndef(V128_i8x16_TYPE))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    /* icmp uge <16 x i8> mask, <16, 16, 16, 16, ...> */
    if (!(max_lane_id =
            build_intx16_vector(comp_ctx, INT8_TYPE, const_lane_ids))) {
        goto fail;
    }

    if (!(condition = LLVMBuildICmp(comp_ctx->builder, LLVMIntUGE, mask,
                                    max_lane_id, "out_of_range"))) {
        HANDLE_FAILURE("LLVMBuldICmp");
        goto fail;
    }

    /*  if the id is out of range (>=16), set the id as 0 */
    if (!(default_lane_value =
            build_intx16_vector(comp_ctx, INT8_TYPE, const_zeors))) {
        goto fail;
    }

    if (!(idx = LLVMBuildSelect(comp_ctx->builder, condition,
                                default_lane_value, mask, "mask"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    for (i = 0; i < 16; i++) {
        if (!(id = LLVMBuildExtractElement(comp_ctx->builder, idx, I8_CONST(i),
                                           "id"))) {
            HANDLE_FAILURE("LLVMBuildExtractElement");
            goto fail;
        }

        if (!(replace_with_zero =
                LLVMBuildExtractElement(comp_ctx->builder, condition,
                                        I8_CONST(i), "replace_with_zero"))) {
            HANDLE_FAILURE("LLVMBuildExtractElement");
            goto fail;
        }

        if (!(elem = LLVMBuildExtractElement(comp_ctx->builder, vector, id,
                                             "vector[mask[i]]"))) {
            HANDLE_FAILURE("LLVMBuildExtractElement");
            goto fail;
        }

        if (!(elem_or_zero =
                LLVMBuildSelect(comp_ctx->builder, replace_with_zero,
                                I8_CONST(0), elem, "elem_or_zero"))) {
            HANDLE_FAILURE("LLVMBuildSelect");
            goto fail;
        }

        if (!(undef =
                LLVMBuildInsertElement(comp_ctx->builder, undef, elem_or_zero,
                                       I8_CONST(i), "new_vector"))) {
            HANDLE_FAILURE("LLVMBuildInsertElement");
            goto fail;
        }
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, undef, V128_i64x2_TYPE,
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
aot_compile_simd_swizzle(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    if (is_target_x86(comp_ctx)) {
        return aot_compile_simd_swizzle_x86(comp_ctx, func_ctx);
    }
    else {
        return aot_compile_simd_swizzle_common(comp_ctx, func_ctx);
    }
}

static bool
aot_compile_simd_extract(AOTCompContext *comp_ctx,
                         AOTFuncContext *func_ctx,
                         uint8 lane_id,
                         bool need_extend,
                         bool is_signed,
                         LLVMTypeRef vector_type,
                         LLVMTypeRef result_type,
                         unsigned aot_value_type)
{
    LLVMValueRef vector, idx, result;

    if (!(idx = I8_CONST(lane_id))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    /* bitcast <2 x i64> %0 to <vector_type> */
    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "vec"))) {
        goto fail;
    }

    /* extractelement <vector_type> %vector, i8 lane_id*/
    if (!(result = LLVMBuildExtractElement(comp_ctx->builder, vector, idx,
                                           "element"))) {
        HANDLE_FAILURE("LLVMBuildExtractElement");
        goto fail;
    }

    if (need_extend) {
        if (is_signed) {
            /* sext <element_type> %element to <result_type> */
            if (!(result = LLVMBuildSExt(comp_ctx->builder, result,
                                         result_type, "ret"))) {
                HANDLE_FAILURE("LLVMBuildSExt");
                goto fail;
            }
        }
        else {
            /* sext <element_type> %element to <result_type> */
            if (!(result = LLVMBuildZExt(comp_ctx->builder, result,
                                         result_type, "ret"))) {
                HANDLE_FAILURE("LLVMBuildZExt");
                goto fail;
            }
        }
    }

    PUSH(result, aot_value_type);

    return true;
fail:
    return false;
}

bool
aot_compile_simd_extract_i8x16(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id,
                               bool is_signed)
{
    return aot_compile_simd_extract(comp_ctx, func_ctx, lane_id, true,
                                    is_signed, V128_i8x16_TYPE, I32_TYPE,
                                    VALUE_TYPE_I32);
}

bool
aot_compile_simd_extract_i16x8(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id,
                               bool is_signed)
{
    return aot_compile_simd_extract(comp_ctx, func_ctx, lane_id, true,
                                    is_signed, V128_i16x8_TYPE, I32_TYPE,
                                    VALUE_TYPE_I32);
}

bool
aot_compile_simd_extract_i32x4(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_extract(comp_ctx, func_ctx, lane_id, false, false,
                                    V128_i32x4_TYPE, I32_TYPE, VALUE_TYPE_I32);
}

bool
aot_compile_simd_extract_i64x2(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_extract(comp_ctx, func_ctx, lane_id, false, false,
                                    V128_i64x2_TYPE, I64_TYPE, VALUE_TYPE_I64);
}

bool
aot_compile_simd_extract_f32x4(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_extract(comp_ctx, func_ctx, lane_id, false, false,
                                    V128_f32x4_TYPE, F32_TYPE, VALUE_TYPE_F32);
}

bool
aot_compile_simd_extract_f64x2(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_extract(comp_ctx, func_ctx, lane_id, false, false,
                                    V128_f64x2_TYPE, F64_TYPE, VALUE_TYPE_F64);
}

static bool
aot_compile_simd_replace(AOTCompContext *comp_ctx,
                         AOTFuncContext *func_ctx,
                         uint8 lane_id,
                         unsigned new_value_type,
                         LLVMTypeRef vector_type,
                         bool need_reduce,
                         LLVMTypeRef element_type)
{
    LLVMValueRef vector, new_value, idx, result;

    POP(new_value, new_value_type);

    if (!(idx = I8_CONST(lane_id))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    /* bitcast <2 x i64> %0 to <vector_type> */

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "vec"))) {
        goto fail;
    }

    /* bitcast <new_value_type> to <element_type> */
    if (need_reduce) {
        if (!(new_value = LLVMBuildTrunc(comp_ctx->builder, new_value,
                                         element_type, "element"))) {
            HANDLE_FAILURE("LLVMBuildTrunc");
            goto fail;
        }
    }

    /* insertelement <vector_type> %vector, <element_type>  %element, i8 idx */
    if (!(result = LLVMBuildInsertElement(comp_ctx->builder, vector, new_value,
                                          idx, "new_vector"))) {
        HANDLE_FAILURE("LLVMBuildInsertElement");
        goto fail;
    }

    /* bitcast <vector_type> %result to <2 x i64> */
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
aot_compile_simd_replace_i8x16(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_replace(comp_ctx, func_ctx, lane_id,
                                    VALUE_TYPE_I32, V128_i8x16_TYPE, true,
                                    INT8_TYPE);
}

bool
aot_compile_simd_replace_i16x8(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_replace(comp_ctx, func_ctx, lane_id,
                                    VALUE_TYPE_I32, V128_i16x8_TYPE, true,
                                    INT16_TYPE);
}

bool
aot_compile_simd_replace_i32x4(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_replace(comp_ctx, func_ctx, lane_id,
                                    VALUE_TYPE_I32, V128_i32x4_TYPE, false,
                                    I32_TYPE);
}

bool
aot_compile_simd_replace_i64x2(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_replace(comp_ctx, func_ctx, lane_id,
                                    VALUE_TYPE_I64, V128_i64x2_TYPE, false,
                                    I64_TYPE);
}

bool
aot_compile_simd_replace_f32x4(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_replace(comp_ctx, func_ctx, lane_id,
                                    VALUE_TYPE_F32, V128_f32x4_TYPE, false,
                                    F32_TYPE);
}

bool
aot_compile_simd_replace_f64x2(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx,
                               uint8 lane_id)
{
    return aot_compile_simd_replace(comp_ctx, func_ctx, lane_id,
                                    VALUE_TYPE_F64, V128_f64x2_TYPE, false,
                                    F64_TYPE);
}
